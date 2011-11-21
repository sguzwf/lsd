#include "settings.h"

#include "details/multicast_server.hpp"
#include "details/helpers.hpp"

namespace pmq {

multicast_server::multicast_server(boost::function<void(std::string)> f) :
	is_running_(false),
	f_(f) {
}

multicast_server::multicast_server(boost::function<void(std::string)> f, const in_addr& iface_ip, const in_addr& multicast_ip, short int port) :
	iface_ip_(iface_ip),
	multicast_ip_(multicast_ip),
	port_(port),
	is_running_(false),
	f_(f) {
}

multicast_server::multicast_server(boost::function<void(std::string)> f, const char* iface_ip, const char* multicast_ip, short int port) :
	port_(port),
	is_running_(false),
	f_(f) {
	set_iface_ip(iface_ip);
	set_multicast_ip(multicast_ip);
}

multicast_server::~multicast_server(){
	stop();
}

bool
multicast_server::is_running() const {
	return is_running_;
}

bool
multicast_server::create_connection() {
	// create a datagram socket to receive data
	sock_datagramm_ = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(sock_datagramm_ < 0) {
		return false;
	}

	// enable SO_REUSEADDR to allow multiple instances of this application to receive
	// copies of the multicast datagrams
	int reuse = 1;
	if (setsockopt(sock_datagramm_, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		close(sock_datagramm_);
		return false;
	}
	
	// bind to the proper port number with the IP address
	memset((char*)&sock_, 0, sizeof(sock_));
	sock_.sin_family = AF_INET;
	sock_.sin_port = htons(port_);
	sock_.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock_datagramm_, (struct sockaddr*)&sock_, sizeof(sock_))) {
		close(sock_datagramm_);
		return false;
	}
	
	// join the multicast group 226.1.1.1 on the local interface
	// note that this IP_ADD_MEMBERSHIP option must be called for each local interface
	// over which the multicast datagrams are to be received.

	group_.imr_multiaddr = multicast_ip_;
	group_.imr_interface = iface_ip_;
	
	if (setsockopt(sock_datagramm_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group_, sizeof(group_)) < 0) {
		close(sock_datagramm_);
		return false;
	}

	return true;
}

void
multicast_server::receive_data() {
	while (is_running_) {
		// read from the socket
		char databuf[1024];
		memset(databuf, 0, sizeof(char) * 1024);

		int datalen = sizeof(databuf);
		if (read(sock_datagramm_, databuf, datalen) >= 0) {
			if (f_) {
				std::string data = std::string(databuf);
				
				if (!data.empty()) {
					f_(data);
				}
			}
		}
		else {
			stop();
		}
	}
}

bool
multicast_server::run() {
	if (!create_connection()) {
		return false;
	}
	
	is_running_ = true;
	thread_ = boost::thread(&multicast_server::receive_data, this);
	
	return true;
}

void
multicast_server::stop() {
	is_running_ = false;
	send_closing_message();
	close(sock_datagramm_);
	thread_.join();
}

void
multicast_server::set_port(unsigned short port) {
	port_ = port;
}

short int
multicast_server::get_port() const {
	return port_;
}

void
multicast_server::set_multicast_ip(const in_addr& ip) {
	multicast_ip_ = ip;
}

void
multicast_server::set_multicast_ip(const char* ip) {
	if (inet_addr(ip) != INADDR_NONE) {
		multicast_ip_.s_addr = inet_addr(ip);
	}
	else {
		throw std::runtime_error("bad ip addr received in multicast_server::set_multicast_ip");
	}
}

in_addr
multicast_server::get_multicast_ip() const {
	return multicast_ip_;
}

void
multicast_server::set_iface_ip(const in_addr& ip) {
	iface_ip_ = ip;
}

void
multicast_server::set_iface_ip(const char* ip) {
	if (inet_addr(ip) != INADDR_NONE) {
		iface_ip_.s_addr = inet_addr(ip);
	}
	else {
		throw std::runtime_error("bad ip addr received in multicast_server::set_iface_ip");
	}
}

in_addr
multicast_server::get_iface_ip() const {
	return iface_ip_;
}

bool
multicast_server::send_closing_message() {
	// create a datagram socket on which to send.
	struct sockaddr_in sock;
	int sock_dm = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_dm < 0) {
		return false;
	}
 
	// initialize the group sockaddr structure with a group address of 225.1.1.1 and port 5555
	memset((char*)&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_addr = multicast_ip_;
	sock.sin_port = htons(port_);
  
	// Set local interface for outbound multicast datagrams. the IP address specified
	// must be associated with a local, multicast capable interface
	if (setsockopt(sock_dm, IPPROTO_IP, IP_MULTICAST_IF, (char *)&iface_ip_, sizeof(iface_ip_)) < 0) {
		return false;
	}

	std::string str = "";
	int len = str.length();
	const char* data = str.c_str();

	if (sendto(sock_dm, data, len, 0, (struct sockaddr*)&sock, sizeof(sock)) < 0) {
		return false;
	}
	
	return true;
}


} // namespace pmq
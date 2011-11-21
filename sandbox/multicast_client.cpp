#include "settings.h"

#include "multicast_client.hpp"
#include "helpers.hpp"

#include <stdexcept>

namespace lsd {

multicast_client::multicast_client() :
	loopback_enabled_(true),
	is_running_(false) {
}

multicast_client::multicast_client(const in_addr& iface_ip, const in_addr& multicast_ip, short int port) :
	iface_ip_(iface_ip),
	multicast_ip_(multicast_ip),
	port_(port),
	loopback_enabled_(true),
	is_running_(false) {
}

multicast_client::multicast_client(const char* iface_ip, const char* multicast_ip, short int port) :
	port_(port),
	loopback_enabled_(true),
	is_running_(false) {
	set_iface_ip(iface_ip);
	set_multicast_ip(multicast_ip);
}

multicast_client::~multicast_client() {
	stop();
	clear_queue();
}

void
multicast_client::set_port(unsigned short port) {
	port_ = port;
}

short int
multicast_client::get_port() const {
	return port_;
}

void
multicast_client::enable_loopback(bool enable) {
	loopback_enabled_ = enable;
}

bool
multicast_client::is_loopback_enabled() const {
	return loopback_enabled_;
}

bool
multicast_client::is_running() const {
	return is_running_;
}

void
multicast_client::set_multicast_ip(const in_addr& ip) {
	multicast_ip_ = ip;
}

void
multicast_client::set_multicast_ip(const char* ip) {
	if (inet_addr(ip) != INADDR_NONE) {
		multicast_ip_.s_addr = inet_addr(ip);
	}
	else {
		throw std::runtime_error("bad ip addr received in multicast_client::set_multicast_ip");
	}
}

in_addr
multicast_client::get_multicast_ip() const {
	return multicast_ip_;
}

void
multicast_client::set_iface_ip(const in_addr& ip) {
	iface_ip_ = ip;
}

void
multicast_client::set_iface_ip(const char* ip) {
	if (inet_addr(ip) != INADDR_NONE) {
		iface_ip_.s_addr = inet_addr(ip);
	}
	else {
		throw std::runtime_error("bad ip addr received in multicast_client::set_iface_ip");
	}
}

in_addr
multicast_client::get_iface_ip() const {
	return iface_ip_;
}

bool
multicast_client::create_connection() {
	// create a datagram socket on which to send.
	sock_datagramm_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_datagramm_ < 0) {
		return false;
	}
 
	// initialize the group sockaddr structure with a group address of 225.1.1.1 and port 5555
	memset((char*)&sock_, 0, sizeof(sock_));
	sock_.sin_family = AF_INET;
	sock_.sin_addr = multicast_ip_;
	sock_.sin_port = htons(port_);
 
	if (!loopback_enabled_) {
		// disable loopback so you do not receive your own datagrams
		char loopch = 0;
		if (setsockopt(sock_datagramm_, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
			close(sock_datagramm_);
			return false;
		}
	}
 
	// Set local interface for outbound multicast datagrams. the IP address specified
	// must be associated with a local, multicast capable interface
	if (setsockopt(sock_datagramm_, IPPROTO_IP, IP_MULTICAST_IF, (char *)&iface_ip_, sizeof(iface_ip_)) < 0) {
		return false;
	}
	
	return true;
}

void
multicast_client::send_message(const std::string& message) {
	if (is_running_ && message_queue_.size() < MAX_QUEUE_SIZE) {
		message_queue_.push(message);
	}
}

void
multicast_client::send_data() {	
	while (is_running_) {
		if (!message_queue_.empty()) {
			int len = message_queue_.front().length();
			const char* data = message_queue_.front().c_str();
			
			if (sendto(sock_datagramm_, data, len, 0, (struct sockaddr*)&sock_, sizeof(sock_)) >= 0) {
				message_queue_.pop();
			}
			else {
				stop();
			}
		}
		else {
			boost::mutex::scoped_lock lock(mutex_);
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);

			xt.sec += 1;
			condition_.timed_wait(lock, xt);
		}
	}
}

bool
multicast_client::run(bool clear_message_queue) {
	if (clear_message_queue) {
		clear_queue();
	}

	stop();
	
	if (!create_connection()) {
		return false;
	}

	is_running_ = true;
	thread_ = boost::thread(&multicast_client::send_data, this);
	
	return true;
}

void
multicast_client::stop() {
	is_running_ = false;
	close(sock_datagramm_);
	thread_.join();
}

void
multicast_client::clear_queue() {
	std::queue<std::string> empty_queue;
	std::swap(message_queue_, empty_queue);
}

} // namespace lsd
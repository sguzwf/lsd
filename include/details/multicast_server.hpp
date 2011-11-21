#ifndef _PMQ_MULTICAST_SERVER_HPP_INCLUDED_
#define _PMQ_MULTICAST_SERVER_HPP_INCLUDED_

#include <iostream>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/thread/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

namespace pmq {

class multicast_server : private boost::noncopyable {
public:
	multicast_server(boost::function<void(std::string)> f);
	multicast_server(boost::function<void(std::string)> f, const in_addr& iface_ip, const in_addr& multicast_ip, short int port);
	multicast_server(boost::function<void(std::string)> f, const char* iface_ip, const char* multicast_ip, short int port);
	virtual ~multicast_server();
	
	bool run();
	void stop();
	bool is_running() const;
	
	void set_iface_ip(const in_addr& ip);
	void set_iface_ip(const char* ip);
	in_addr get_iface_ip() const;
	
	void set_multicast_ip(const in_addr& ip);
	void set_multicast_ip(const char* ip);
	in_addr get_multicast_ip() const;
	
	void set_port(unsigned short port);
	short int get_port() const;
	
private:
	bool create_connection();
	void receive_data();
	bool send_closing_message();

private:
	struct ip_mreq group_;
	struct in_addr iface_ip_;
	struct in_addr multicast_ip_;
	unsigned short port_;
	struct sockaddr_in sock_;
	int sock_datagramm_;
	
	boost::thread thread_;
	boost::mutex mutex_;
	volatile bool is_running_;
	
	boost::function<void(std::string)> f_;
};
	
} // namespace pmq

#endif // _PMQ_MULTICAST_SERVER_HPP_INCLUDED_
#ifndef _PMQ_MULTICAST_CLIENT_HPP_INCLUDED_
#define _PMQ_MULTICAST_CLIENT_HPP_INCLUDED_

#include <iostream>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#include <queue>

namespace pmq {

class multicast_client : private boost::noncopyable {

public:
	multicast_client();
	multicast_client(const in_addr& iface_ip, const in_addr& multicast_ip, short int port);
	multicast_client(const char* iface_ip, const char* multicast_ip, short int port);
	virtual ~multicast_client();

	bool run(bool clear_message_queue = true);
	void stop();
	bool is_running() const;

	void send_message(const std::string& message);

	void set_iface_ip(const in_addr& ip);
	void set_iface_ip(const char* ip);
	in_addr get_iface_ip() const;
	
	void set_multicast_ip(const in_addr& ip);
	void set_multicast_ip(const char* ip);
	in_addr get_multicast_ip() const;
	
	void set_port(unsigned short port);
	short int get_port() const;

	void enable_loopback(bool enable);
	bool is_loopback_enabled() const;

	static const size_t MAX_QUEUE_SIZE = 1000;

private:
	bool create_connection();
	void send_data();
	void clear_queue();
	
private:
	struct in_addr iface_ip_;
	struct in_addr multicast_ip_;
	unsigned short port_;
	bool loopback_enabled_;
	struct sockaddr_in sock_;
	int sock_datagramm_;
	
	std::queue<std::string> message_queue_;
	
	boost::thread thread_;
	boost::condition condition_;
	boost::mutex mutex_;
	volatile bool is_running_;

};
	
} // namespace pmq

#endif // _PMQ_MULTICAST_CLIENT_HPP_INCLUDED_
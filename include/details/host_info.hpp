#ifndef _LSD_HOST_INFO_HPP_INCLUDED_
#define _LSD_HOST_INFO_HPP_INCLUDED_

#include <iostream>
#include <string>
#include <stdexcept>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include <cerrno>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "lsd/structs.hpp"

namespace lsd {

// predeclaration
template <typename LSD_T> class host_info;
typedef host_info<LT> host_info_t;

template<typename LSD_T>
class host_info {
public:
	host_info() : ip_(0) {
	}
	
	explicit host_info(const typename LSD_T::ip_addr ip) :
	ip_(ip) {
		hostname_ = hostname_for_ip(ip);
	}
	
	explicit host_info(const std::string& ip) {
		ip_ = ip_from_string(ip);
		hostname_ = hostname_for_ip(ip);
	}

	host_info(const host_info<LSD_T>& info) :
	ip_(info.ip_), hostname_(info.hostname_) {
	}
	
	host_info(const typename LSD_T::ip_addr ip, const std::string& hostname) :
		ip_(ip), hostname_(hostname) {
	}
	
	bool operator == (const host_info<LSD_T>& info) {
		return (ip_ == info.ip_ && hostname_ == info.hostname_);
	}

	static typename LSD_T::ip_addr ip_from_string(const std::string& ip) {
		typename LSD_T::ip_addr addr;
        int res = inet_pton(AF_INET, ip.c_str(), &addr);
		
        if (0 == res) {
			throw std::runtime_error(std::string("bad ip address ") + ip);
        }
        else if (res < 0) {
			throw std::runtime_error("bad address translation");
        }

        return htonl(addr);
	}
	
	static std::string string_from_ip(const typename LSD_T::ip_addr& ip) {
		char buf[128];
        typename LSD_T::ip_addr n = ntohl(ip);
        return inet_ntop(AF_INET, &n, buf, sizeof (buf));
	}
	
	static std::string hostname_for_ip(const std::string& ip) {
		in_addr_t data;
		data = inet_addr(ip.c_str());
		const hostent* host_info = gethostbyaddr(&data, 4, AF_INET);
		if (host_info) {
			return std::string(host_info->h_name);
		}
		
		return "";
	}
	
	static std::string hostname_for_ip(const typename LSD_T::ip_addr& ip) {
		return hostname_for_ip(string_from_ip(ip));
	}
	
	std::string as_string() {
		return string_from_ip(ip_) + " (" + hostname_ + ")";
	}

	typename LSD_T::ip_addr ip_;
	std::string hostname_;
};

template <typename LSD_T>
std::ostream& operator << (std::ostream& out, const host_info<LSD_T>& host) {
	out << host_info<LSD_T>::string_from_ip(host.ip_) << " (" << host.hostname_ << ")";
	return out;
};

} // namespace lsd

#endif // _LSD_HOST_INFO_HPP_INCLUDED_

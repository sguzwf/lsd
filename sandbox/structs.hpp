#ifndef _LSD_STRUCTS_HPP_INCLUDED_
#define _LSD_STRUCTS_HPP_INCLUDED_

#include <vector>
#include <stdexcept>
#include <time.h>
#include <boost/cstdint.hpp>

namespace lsd {

static const int PROTOCOL_VERSION = 1;
static const unsigned long long MESSAGE_TIMEOUT = 10;	// seconds
static const unsigned long long HEARTBEAT_INTERVAL = 1;	// seconds
static const unsigned long long DEFAULT_SOCKET_POLL_TIMEOUT = 2000; // milliseconds
static const unsigned long long DEFAULT_SOCKET_PING_TIMEOUT = 500; // milliseconds

static const std::string DEFAULT_EBLOB_PATH = "/tmp/pmq_eblob";
static const std::string DEFAULT_EBLOB_LOG_PATH = "/var/log/pmq_eblob.log";
static const unsigned int DEFAULT_EBLOB_LOG_FLAGS = 0;
static const int DEFAULT_EBLOB_SYNC_INTERVAL = 1;

static const std::string DEFAULT_HOSTS_URL = "";
static const unsigned short DEFAULT_CONTROL_PORT = 5555;
static const std::string DEFAULT_MULTICAST_IP = "226.1.1.1";
static const unsigned short DEFAULT_MULTICAST_PORT = 5556;

enum logger_type {
	STDOUT_LOGGER = 1,
	FILE_LOGGER,
	SYSLOG_LOGGER
};

struct lsd_types {
	typedef boost::uint32_t ip_addr;
	typedef boost::uint16_t port;
};


struct host_heartbeat {
	host_heartbeat() {		
	}

	//host_heartbeat(const std::string& ip, const std::string& hostname) : 
	//host_(ip, hostname), last_heartbeat_time_(time(NULL)) {
	//}
	
	//bool operator==(const host_heartbeat& heartbeat) {
	//	return (host_ == heartbeat.host_);
	//}

	//host_info host_;
	//time_t last_heartbeat_time_;
};


enum autodiscovery_type {
	AT_MULTICAST = 1,
	AT_HTTP
};

} // namespace lsd

#endif // _LSD_STRUCTS_HPP_INCLUDED_
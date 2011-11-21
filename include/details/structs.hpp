#ifndef _PQM_STRUCTS_HPP_INCLUDED_
#define _PQM_STRUCTS_HPP_INCLUDED_

#include <vector>
#include <time.h>

namespace pmq {

static const int HEARTBEAT_INTERVAL = 1;	// seconds
static const int PROTOCOL_VERSION = 1;
static const double MESSAGE_TIMEOUT = 10.0;	// seconds
static const int DEFAULT_SOCKET_POLL_TIMEOUT = 2000;

static const std::string DEFAULT_EBLOB_PATH = "/tmp/pmq_eblob";
static const std::string DEFAULT_EBLOB_LOG_PATH = "/var/log/pmq_eblob.log";
static const unsigned int DEFAULT_EBLOB_LOG_FLAGS = 0;
static const int DEFAULT_EBLOB_SYNC_INTERVAL = 1;

static const std::string DEFAULT_CONDUCTOR_URL = "http://c.yandex-team.ru/api/groups2hosts/";
static const unsigned short DEFAULT_CONTROL_PORT = 5555;
static const std::string DEFAULT_MULTICAST_IP = "226.1.1.1";
static const unsigned short DEFAULT_MULTICAST_PORT = 5556;

enum logger_type {
	STDOUT_LOGGER = 1,
	FILE_LOGGER,
	SYSLOG_LOGGER
};

struct host_info {
	host_info () {
	}

	host_info(const std::string& ip, const std::string& hostname) : 
	ip_(ip), name_(hostname) {
	}
	
	bool operator==(const host_info& info) {
		return (ip_ == info.ip_ && name_ == info.name_);
	}

	std::string ip_;
	std::string name_;
};

struct host_heartbeat {
	host_heartbeat() {		
	}

	host_heartbeat(const std::string& ip, const std::string& hostname) : 
	host_(ip, hostname), last_heartbeat_time_(time(NULL)) {
	}
	
	bool operator==(const host_heartbeat& heartbeat) {
		return (host_ == heartbeat.host_);
	}

	host_info host_;
	time_t last_heartbeat_time_;
};

struct service_info {
	service_info() {
	}

	service_info (const std::string& name, const std::string& prefix, const std::string& conductor_name) :
	name_(name), prefix_(prefix), conductor_name_(conductor_name), messages_port_(0) {		
	}

	std::string name_;
	std::string prefix_;
	std::string conductor_name_;
	unsigned short messages_port_;
};

enum autodiscovery_type {
	AT_MULTICAST = 1,
	AT_CONDUCTOR
};

} // namespace pmq

#endif // _PQM_STRUCTS_HPP_INCLUDED_
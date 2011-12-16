//
// Copyright (C) 2011 Rim Zaidullin <creator@bash.org.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef _LSD_STRUCTS_HPP_INCLUDED_
#define _LSD_STRUCTS_HPP_INCLUDED_

#include <vector>
#include <stdexcept>
#include <time.h>
#include <boost/cstdint.hpp>

namespace lsd {

static const int PROTOCOL_VERSION = 1;
static const int STATISTICS_PROTOCOL_VERSION = 1;
static const unsigned long long MESSAGE_TIMEOUT = 10;	// seconds
static const unsigned long long HEARTBEAT_INTERVAL = 1;	// seconds
static const unsigned long long DEFAULT_SOCKET_POLL_TIMEOUT = 2000; // milliseconds
static const unsigned long long DEFAULT_SOCKET_PING_TIMEOUT = 1000; // milliseconds

static const std::string DEFAULT_EBLOB_PATH = "/tmp/pmq_eblob";
static const std::string DEFAULT_EBLOB_LOG_PATH = "/var/log/pmq_eblob.log";
static const unsigned int DEFAULT_EBLOB_LOG_FLAGS = 0;
static const int DEFAULT_EBLOB_SYNC_INTERVAL = 1;

static const std::string DEFAULT_HOSTS_URL = "";
static const unsigned short DEFAULT_CONTROL_PORT = 5555;
static const std::string DEFAULT_MULTICAST_IP = "226.1.1.1";
static const unsigned short DEFAULT_MULTICAST_PORT = 5556;
static const unsigned short DEFAULT_STATISTICS_PORT = 3333;
static const size_t DEFAULT_MAX_MESSAGE_CACHE_SIZE = 512; // megabytes



enum logger_type {
	STDOUT_LOGGER = 1,
	FILE_LOGGER,
	SYSLOG_LOGGER
};

enum autodiscovery_type {
	AT_MULTICAST = 1,
	AT_HTTP
};

enum message_cache_type {
	RAM_ONLY = 1,
	PERSISTANT
};

struct message_path {
	message_path() {};
	message_path(const std::string& service_name_,
				 const std::string& handle_name_) :
		service_name(service_name_),
		handle_name(handle_name_) {};

	message_path(const message_path& path) :
		service_name(path.service_name),
		handle_name(path.handle_name) {};

	message_path& operator = (const message_path& rhs) {
		if (this == &rhs) {
			return *this;
		}

		service_name = rhs.service_name;
		handle_name = rhs.handle_name;

		return *this;
	}

	bool operator == (const message_path& mp) const {
		return (service_name == mp.service_name &&
				handle_name == mp.handle_name);
	}

	bool operator != (const message_path& mp) const {
		return !(*this == mp);
	}

	size_t container_size() const {
		return (service_name.length() + handle_name.length());
	}

	std::string service_name;
	std::string handle_name;
};

struct message_policy {
	message_policy() :
		send_to_all_hosts(false),
		urgent(false),
		mailboxed(false),
		timeout(0.0f),
		deadline(0.0f),
		max_timeout_retries(0) {};

	message_policy(bool send_to_all_hosts_,
				   bool urgent_,
				   float mailboxed_,
				   float timeout_,
				   float deadline_,
				   int max_timeout_retries_) :
		send_to_all_hosts(send_to_all_hosts_),
		urgent(urgent_),
		mailboxed(mailboxed_),
		timeout(timeout_),
		deadline(deadline_),
		max_timeout_retries(max_timeout_retries_) {};

	message_policy(const message_policy& mp) {
		*this = mp;
	}

	message_policy& operator = (const message_policy& rhs) {
		if (this == &rhs) {
			return *this;
		}

		send_to_all_hosts = rhs.send_to_all_hosts;
		urgent = rhs.urgent;
		mailboxed = rhs.mailboxed;
		timeout = rhs.timeout;
		deadline = rhs.deadline;
		max_timeout_retries = rhs.max_timeout_retries;

		return *this;
	}

	bool operator == (const message_policy& rhs) const {
		return (send_to_all_hosts == rhs.send_to_all_hosts &&
				urgent == rhs.urgent &&
				mailboxed == rhs.mailboxed &&
				timeout == rhs.timeout &&
				deadline == rhs.deadline);
	}

	bool operator != (const message_policy& rhs) const {
		return !(*this == rhs);
	}

	bool send_to_all_hosts;
    bool urgent;
    bool mailboxed;
    double timeout;
    double deadline;
    int max_timeout_retries;
};

struct lsd_types {
	typedef boost::uint32_t ip_addr;
	typedef boost::uint16_t port;
};

typedef lsd_types LT;

} // namespace lsd

#endif // _LSD_STRUCTS_HPP_INCLUDED_

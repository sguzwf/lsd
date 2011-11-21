#ifndef _LSD_CONFIG_HPP_INCLUDED_
#define _LSD_CONFIG_HPP_INCLUDED_

#include <string>
#include <map>
#include <iostream>

#include "structs.hpp"
#include "globals.hpp"
#include "smart_logger.hpp"

namespace lsd {

class configuration;
	
std::ostream& operator<<(std::ostream& out, configuration& config);
	
class configuration {
public:
	// map lsd service name to service info
	typedef std::map<std::string, service_info_t> services_list_t;

public:
	configuration();
	configuration(const std::string& path);
	virtual ~configuration();
	
	void load(const std::string& path);
	
	const std::string& config_path() const;
	unsigned int config_version() const;
	unsigned long long message_timeout() const;
	unsigned long long socket_poll_timeout() const;
	
	enum logger_type logger_type() const;
	unsigned int logger_flags() const;
	const std::string& logger_file_path() const;
	const std::string& logger_syslog_name() const;
	
	std::string eblob_path() const;
	std::string eblob_log_path() const;
	unsigned int eblob_log_flags() const;
	int eblob_sync_interval() const;
	
	enum autodiscovery_type autodiscovery_type() const;
	std::string multicast_ip() const;
	unsigned short multicast_port() const;
	
	const services_list_t& services_list() const;
	service_info_t service_by_prefix(const std::string& service_prefix) const;
	
	friend std::ostream& operator<<(std::ostream& out, configuration& config);
	
private:
	// config
	std::string path_;
	unsigned int version_;
	
	// general
	unsigned long long message_timeout_;
	unsigned long long socket_poll_timeout_;
	
	// logger
	enum logger_type logger_type_;
	unsigned int logger_flags_;
	std::string logger_file_path_;
	std::string logger_syslog_name_;

	// persistent storage
	std::string eblob_path_;
	std::string eblob_log_path_;
	unsigned int eblob_log_flags_;
	int eblob_sync_interval_;
	
	// autodiscovery
	enum autodiscovery_type autodiscovery_type_;
	std::string multicast_ip_;
	unsigned short multicast_port_;
	
	// services
	services_list_t services_list_;
};

} // namespace lsd

#endif // _LSD_CONFIG_HPP_INCLUDED_

#ifndef _PMQ_CONFIG_HPP_INCLUDED_
#define _PMQ_CONFIG_HPP_INCLUDED_

#include <string>
#include <map>
#include <iostream>

#include "structs.hpp"
#include "smart_logger.hpp"

namespace pmq {

class configuration;
	
std::ostream& operator<<(std::ostream& out, configuration& config);
	
class configuration {
public:
	configuration();
	configuration(const std::string& path);
	virtual ~configuration();
	
	void load(const std::string& path);
	
	const std::string& config_path() const;
	unsigned int config_version() const;
	double message_timeout() const;
	int socket_poll_timeout() const;
	
	enum logger_type logger_type() const;
	unsigned int logger_flags() const;
	const std::string& logger_file_path() const;
	const std::string& logger_syslog_name() const;
	
	std::string eblob_path() const;
	std::string eblob_log_path() const;
	unsigned int eblob_log_flags() const;
	int eblob_sync_interval() const;
	
	enum autodiscovery_type autodiscovery_type() const;
	std::string conductor_url() const;
	unsigned short control_port() const;
	std::string multicast_ip() const;
	unsigned short multicast_port() const;
	
	const std::map<std::string, service_info>& services_list() const;
	service_info service_by_prefix(const std::string& service_prefix) const;
	
	friend std::ostream& operator<<(std::ostream& out, configuration& config);
	
private:
	// config
	std::string path_;
	unsigned int version_;
	
	// general
	double message_timeout_;
	int socket_poll_timeout_;
	
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
	std::string conductor_url_;
	unsigned short control_port_;
	std::string multicast_ip_;
	unsigned short multicast_port_;
	
	// services
	std::map<std::string, service_info> services_list_;
};

} // namespace pmq

#endif // _PMQ_CONFIG_HPP_INCLUDED_
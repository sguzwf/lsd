#ifndef _LSD_CONFIG_HPP_INCLUDED_
#define _LSD_CONFIG_HPP_INCLUDED_

#include <string>
#include <map>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/utility.hpp>

#include "lsd/structs.hpp"
#include "details/service_info.hpp"
#include "details/smart_logger.hpp"

namespace lsd {

class configuration;
	
std::ostream& operator << (std::ostream& out, configuration& config);
	
class configuration : private boost::noncopyable {
public:
	// map lsd service name to service info
	typedef std::map<std::string, service_info_t> services_list_t;

public:
	configuration();
	explicit configuration(const std::string& path);
	virtual ~configuration();
	
	void load(const std::string& path);
	
	const std::string& config_path() const;
	unsigned int config_version() const;
	unsigned long long message_timeout() const;
	unsigned long long socket_poll_timeout() const;
	size_t max_message_cache_size() const;
	enum message_cache_type message_cache_type() const;
	
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
	bool service_info_by_name(const std::string& name, service_info_t& info) const;
	bool service_info_by_name(const std::string& name) const;
	
	friend std::ostream& operator<<(std::ostream& out, configuration& config);
	
private:
	// config
	std::string path_;
	unsigned int version_;
	
	// general
	unsigned long long message_timeout_;
	unsigned long long socket_poll_timeout_;
	size_t max_message_cache_size_;
	enum message_cache_type message_cache_type_;
	
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

	// synchronization
	boost::mutex mutex_;
};

} // namespace lsd

#endif // _LSD_CONFIG_HPP_INCLUDED_

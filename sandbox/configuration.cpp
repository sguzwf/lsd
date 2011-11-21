#include "settings.h"

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <stdexcept>
#include "json/json.h"

#include "configuration.hpp"
#include "structs.hpp"

namespace lsd {

configuration::configuration() :
	version_ (0),
	message_timeout_(MESSAGE_TIMEOUT),
	socket_poll_timeout_(DEFAULT_SOCKET_POLL_TIMEOUT),
	logger_type_(STDOUT_LOGGER),
	logger_flags_(PLOG_NONE),
	eblob_path_(DEFAULT_EBLOB_PATH),
	eblob_log_path_(DEFAULT_EBLOB_LOG_PATH),
	eblob_log_flags_(DEFAULT_EBLOB_LOG_FLAGS),
	eblob_sync_interval_(DEFAULT_EBLOB_SYNC_INTERVAL),
	autodiscovery_type_(AT_HTTP),
	multicast_ip_(DEFAULT_MULTICAST_IP),
	multicast_port_(DEFAULT_MULTICAST_PORT)
{
	
}

configuration::configuration(const std::string& path) :
	path_(path),
	version_ (0),
	message_timeout_(MESSAGE_TIMEOUT),
	socket_poll_timeout_(DEFAULT_SOCKET_POLL_TIMEOUT),
	logger_type_(STDOUT_LOGGER),
	logger_flags_(PLOG_NONE),
	eblob_path_(DEFAULT_EBLOB_PATH),
	eblob_log_path_(DEFAULT_EBLOB_LOG_PATH),
	eblob_log_flags_(DEFAULT_EBLOB_LOG_FLAGS),
	eblob_sync_interval_(DEFAULT_EBLOB_SYNC_INTERVAL),
	autodiscovery_type_(AT_HTTP),
	multicast_ip_(DEFAULT_MULTICAST_IP),
	multicast_port_(DEFAULT_MULTICAST_PORT)
{
	load(path);
}

configuration::~configuration() {
	
}

void
configuration::load(const std::string& path) {
	path_ = path;

	std::ifstream file(path.c_str(), std::ifstream::in);
	
	if (!file.is_open()) {
		throw std::runtime_error("config file: " + path + " failed to open at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	std::string config_data;
	std::string line;
	while (std::getline(file, line)) {
		config_data += line;// + "\n";
	}
	
	file.close();

	Json::Value root;
	Json::Reader reader;
	bool parsing_successful = reader.parse(config_data, root);
		
	if (!parsing_successful) {
		throw std::runtime_error("config file: " + path + " could not be parsed at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
	
	const Json::Value config_value = root["lsd_config"];
	
	try {
		version_ = config_value.get("config_version", 0).asUInt();
		message_timeout_ = (unsigned long long)config_value.get("message_timeout", (int)MESSAGE_TIMEOUT).asInt();
		socket_poll_timeout_ = (unsigned long long)config_value.get("socket_poll_timeout", (int)DEFAULT_SOCKET_POLL_TIMEOUT).asInt();
		
		std::string log_type = config_value.get("log_type", "STDOUT_LOGGER").asString();
		
		if (log_type == "STDOUT_LOGGER") {
			logger_type_ = STDOUT_LOGGER;
		}
		else if (log_type == "FILE_LOGGER") {
			logger_type_ = FILE_LOGGER;
		}
		else if (log_type == "SYSLOG_LOGGER") {
			logger_type_ = SYSLOG_LOGGER;
		}
		
		std::string log_flags = config_value.get("log_flags", "PLOG_NONE").asString();
		
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("|");
		tokenizer tokens(log_flags, sep);

		for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
			std::string flag = *tok_iter;
			boost::trim(flag);
			
			if (flag == "PLOG_NONE") {
				logger_flags_ |= PLOG_NONE;
			}
			else if (flag == "PLOG_INFO") {
				logger_flags_ |= PLOG_INFO;
			}
			else if (flag == "PLOG_DEBUG") {
				logger_flags_ |= PLOG_DEBUG;
			}
			else if (flag == "PLOG_WARNING") {
				logger_flags_ |= PLOG_WARNING;
			}
			else if (flag == "PLOG_ERROR") {
				logger_flags_ |= PLOG_ERROR;
			}
			else if (flag == "PLOG_MSG_TYPES") {
				logger_flags_ |= PLOG_MSG_TYPES;
			}
			else if (flag == "PLOG_ALL") {
				logger_flags_ |= PLOG_ALL;
			}
		}
		
		logger_file_path_  = config_value.get("log_file", "").asString();
		logger_syslog_name_  = config_value.get("log_syslog_name", "").asString();
		
		const Json::Value persistent_storage_value = config_value["persistent_storage"];
		
		eblob_path_ = persistent_storage_value.get("eblob_path", "").asString();
		eblob_log_path_ = persistent_storage_value.get("eblob_log_path", "").asString();
		eblob_log_flags_ = persistent_storage_value.get("eblob_log_flags", 0).asUInt();
		eblob_sync_interval_ = persistent_storage_value.get("eblob_sync_interval", DEFAULT_EBLOB_SYNC_INTERVAL).asInt();
		
		const Json::Value autodiscovery_value = config_value["autodiscovery"];
		
		std::string atype = autodiscovery_value.get("type", "HTTP").asString();
		if (atype == "HTTP") {
			autodiscovery_type_ = AT_HTTP;
		}
		else if (atype == "MULTICAST") {
			autodiscovery_type_ = AT_MULTICAST;
		}

		multicast_ip_ = autodiscovery_value.get("multicast_ip", DEFAULT_MULTICAST_IP).asString();
		multicast_port_ = autodiscovery_value.get("multicast_port", DEFAULT_MULTICAST_PORT).asUInt();
		
		const Json::Value services_value = config_value["services"];
		
		service_info_t si;
		
		for (size_t index = 0; index < services_value.size(); ++index) {
			const Json::Value service_value = services_value[index];
			si.description_ = service_value.get("description", "").asString();
			si.name_ = service_value.get("name", "").asString();
			si.app_name_ = service_value.get("app_name", "").asString();
			si.hosts_url_ = service_value.get("hosts_url", "").asString();
			si.control_port_ = service_value.get("control_port", DEFAULT_CONTROL_PORT).asUInt();
			
			// check values for validity
			if (si.name_.empty()) {
				throw std::runtime_error("service with no name was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
			}

			if (si.app_name_.empty()) {
				throw std::runtime_error("service with no application name was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
			}
			
			if (si.hosts_url_.empty()) {
				throw std::runtime_error("service with no hosts_url was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
			}
			
			if (si.control_port_ == 0) {
				throw std::runtime_error("service with no control port == 0 was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
			}
			
			// check for duplicate services

			// no service can have the same name
			std::map<std::string, service_info_t>::iterator it = services_list_.begin();
			for (;it != services_list_.end(); ++it) {
				if (it->second.name_ == si.name_) {
					throw std::runtime_error("duplicate service with name " + si.name_ + " was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
				}
			}

			// no service can have the same app_name + control_port
			it = services_list_.begin();
			for (;it != services_list_.end(); ++it) {
				if (it->second.app_name_ == si.app_name_ && it->second.control_port_ == si.control_port_) {
					std::string err_msg = "duplicate service with app name " + si.app_name_ + " and ";
					err_msg += "control port " + boost::lexical_cast<std::string>(si.control_port_) + " was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION);
					throw std::runtime_error(err_msg);
				}
			}

			services_list_[si.name_] = si;
		}
	}
	catch (...) {
		throw std::runtime_error("config file: " + path + " could not be parsed at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

const std::string&
configuration::config_path() const {
	return path_;
}

unsigned int
configuration::config_version() const {
	return version_;
}

unsigned long long
configuration::message_timeout() const {
	return message_timeout_;
}

unsigned long long
configuration::socket_poll_timeout() const {
	return socket_poll_timeout_;
}

enum logger_type
configuration::logger_type() const {
	return logger_type_;
}

unsigned int
configuration::logger_flags() const {
	return logger_flags_;
}

const std::string&
configuration::logger_file_path() const {
	return logger_file_path_;
}

const std::string&
configuration::logger_syslog_name() const {
	return logger_syslog_name_;
}

std::string
configuration::eblob_path() const {
	return eblob_path_;
}

std::string
configuration::eblob_log_path() const {
	return eblob_log_path_;
}

unsigned int
configuration::eblob_log_flags() const {
	return eblob_log_flags_;
}

int
configuration::eblob_sync_interval() const {
	return eblob_sync_interval_;
}

enum autodiscovery_type
configuration::autodiscovery_type() const {
	return autodiscovery_type_;
}

std::string
configuration::multicast_ip() const {
	return multicast_ip_;
}

unsigned short
configuration::multicast_port() const {
	return multicast_port_;
}

const std::map<std::string, service_info_t>&
configuration::services_list() const {
	return services_list_;
}

service_info_t
configuration::service_by_prefix(const std::string& service_prefix) const {
	std::map<std::string, service_info_t>::const_iterator it = services_list_.find(service_prefix);
	
	if (it != services_list_.end()) {
		return it->second;
	}
	else {
		throw std::runtime_error("no info for service with prefix " + service_prefix + " at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

std::ostream& operator<<(std::ostream& out, configuration& config) {
	out << "config path: " << config.path_ << "\n";
	out << "config version: " << config.version_ << "\n\n";
	
	out << "message timeout: " << config.message_timeout_ << "\n";
	out << "socket poll timeout: " << config.socket_poll_timeout_ << "\n\n";
	
	if (config.logger_type_ == STDOUT_LOGGER) {
		out << "logger type: STDOUT_LOGGER" << "\n";
	}
	else if (config.logger_type_ == FILE_LOGGER) {
		out << "logger type: FILE_LOGGER" << "\n";
	}
	else if (config.logger_type_ == SYSLOG_LOGGER) {
		out << "logger type: SYSLOG_LOGGER" << "\n";
	}
	
	if (config.logger_flags_ == PLOG_NONE) {
		out << "logger flags: PLOG_NONE" << "\n";
	}
	else if (config.logger_flags_ == PLOG_ALL) {
		out << "logger flags: PLOG_ALL" << "\n";
	}
	else {
		out << "logger flags: ";
		
		if ((config.logger_flags_ & PLOG_INFO) == PLOG_INFO) {
			out << "PLOG_INFO ";
		}
		
		if ((config.logger_flags_ & PLOG_DEBUG) == PLOG_DEBUG) {
			out << "PLOG_DEBUG ";
		}
		
		if ((config.logger_flags_ & PLOG_WARNING) == PLOG_WARNING) {
			out << "PLOG_WARNING ";
		}
		
		if ((config.logger_flags_ & PLOG_ERROR) == PLOG_ERROR) {
			out << "PLOG_ERROR ";
		}

		if ((config.logger_flags_ & PLOG_MSG_TYPES) == PLOG_MSG_TYPES) {
			out << "PLOG_MSG_TYPES ";
		}

		out << "\n";
	}

	out << "logger file path: " << config.logger_file_path_ << "\n";
 	out << "logger syslog name: " << config.logger_syslog_name_ << "\n\n";

	out << "eblob path: " << config.eblob_path_ << "\n";
	out << "eblob log path: " << config.eblob_log_path_ << "\n";
	out << "eblob log flags: " << config.eblob_log_flags_ << "\n";
 	out << "eblob sync interval: " << config.eblob_sync_interval_ << "\n\n";
	
	if (config.autodiscovery_type_ == AT_MULTICAST) {
		out << "autodiscovery type: MULTICAST" << "\n";
	}
	else if (config.autodiscovery_type_ == AT_HTTP) {
		out << "autodiscovery type: HTTP" << "\n";
	}

	out << "multicast ip: " << config.multicast_ip_ << "\n";
	out << "multicast port: " << config.multicast_port_ << "\n\n";
	
	out << "services: " << "\n";
	std::map<std::string, service_info_t>& sl = config.services_list_;
	
	for (std::map<std::string, service_info_t>::iterator it = sl.begin(); it != sl.end(); ++it) {
		out << "\n    description: " << it->second.description_ << "\n";
		out << "    name: " << it->second.name_ << "\n";
		out << "    hosts url: " << it->second.hosts_url_ << "\n";
		out << "    control port: " << it->second.control_port_ << "\n";
	}
	
	return out;
}

} // namespace lsd
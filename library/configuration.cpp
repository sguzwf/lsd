#include <fstream>
#include <stdexcept>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include "details/configuration.hpp"

namespace lsd {

configuration::configuration() :
	version_ (0),
	message_timeout_(MESSAGE_TIMEOUT),
	socket_poll_timeout_(DEFAULT_SOCKET_POLL_TIMEOUT),
	max_message_cache_size_(DEFAULT_MAX_MESSAGE_CACHE_SIZE),
	logger_type_(STDOUT_LOGGER),
	logger_flags_(PLOG_NONE),
	eblob_path_(DEFAULT_EBLOB_PATH),
	eblob_log_path_(DEFAULT_EBLOB_LOG_PATH),
	eblob_log_flags_(DEFAULT_EBLOB_LOG_FLAGS),
	eblob_sync_interval_(DEFAULT_EBLOB_SYNC_INTERVAL),
	autodiscovery_type_(AT_HTTP),
	multicast_ip_(DEFAULT_MULTICAST_IP),
	multicast_port_(DEFAULT_MULTICAST_PORT),
	is_statistics_enabled_(false),
	is_remote_statistics_enabled_(false),
	remote_statistics_port_(DEFAULT_STATISTICS_PORT)
{
	
}

configuration::configuration(const std::string& path) :
	path_(path),
	version_ (0),
	message_timeout_(MESSAGE_TIMEOUT),
	socket_poll_timeout_(DEFAULT_SOCKET_POLL_TIMEOUT),
	max_message_cache_size_(DEFAULT_MAX_MESSAGE_CACHE_SIZE),
	logger_type_(STDOUT_LOGGER),
	logger_flags_(PLOG_NONE),
	eblob_path_(DEFAULT_EBLOB_PATH),
	eblob_log_path_(DEFAULT_EBLOB_LOG_PATH),
	eblob_log_flags_(DEFAULT_EBLOB_LOG_FLAGS),
	eblob_sync_interval_(DEFAULT_EBLOB_SYNC_INTERVAL),
	autodiscovery_type_(AT_HTTP),
	multicast_ip_(DEFAULT_MULTICAST_IP),
	multicast_port_(DEFAULT_MULTICAST_PORT),
	is_statistics_enabled_(false),
	is_remote_statistics_enabled_(false),
	remote_statistics_port_(DEFAULT_STATISTICS_PORT)
{
	load(path);
}

configuration::~configuration() {
	
}

void
configuration::parse_basic_settings(const Json::Value& config_value) {
	version_ = config_value.get("config_version", 0).asUInt();
	message_timeout_ = (unsigned long long)config_value.get("message_timeout", (int)MESSAGE_TIMEOUT).asInt();
	socket_poll_timeout_ = (unsigned long long)config_value.get("socket_poll_timeout", (int)DEFAULT_SOCKET_POLL_TIMEOUT).asInt();
}

void
configuration::parse_logger_settings(const Json::Value& config_value) {
	const Json::Value logger_value = config_value["logger"];
	std::string log_type = logger_value.get("type", "STDOUT_LOGGER").asString();

	if (log_type == "STDOUT_LOGGER") {
		logger_type_ = STDOUT_LOGGER;
	}
	else if (log_type == "FILE_LOGGER") {
		logger_type_ = FILE_LOGGER;
	}
	else if (log_type == "SYSLOG_LOGGER") {
		logger_type_ = SYSLOG_LOGGER;
	}

	std::string log_flags = logger_value.get("flags", "PLOG_NONE").asString();

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
		else if (flag == "PLOG_MSG_TIME") {
			logger_flags_ |= PLOG_MSG_TIME;
		}
	}

	logger_file_path_  = logger_value.get("file", "").asString();
	logger_syslog_name_  = logger_value.get("syslog_name", "").asString();
}

void
configuration::parse_messages_cache_settings(const Json::Value& config_value) {
	const Json::Value cache_value = config_value["message_cache"];
	max_message_cache_size_ = (size_t)cache_value.get("max_ram_limit", (int)DEFAULT_MAX_MESSAGE_CACHE_SIZE).asInt();
	max_message_cache_size_ *= 1048576; // convert mb to bytes

	std::string message_cache_type_str = cache_value.get("type", "RAM_ONLY").asString();

	if (message_cache_type_str == "PERSISTANT") {
		message_cache_type_ = PERSISTANT;
	}
	else if (message_cache_type_str == "RAM_ONLY") {
		message_cache_type_ = RAM_ONLY;
	}
	else {
		std::string error_str = "unknown message cache type: " + message_cache_type_str;
		error_str += "message_cache/type property can only take RAM_ONLY or PERSISTANT as value. ";
		error_str += "at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}
}

void
configuration::parse_persistant_storage_settings(const Json::Value& config_value) {
	const Json::Value persistent_storage_value = config_value["persistent_storage"];

	eblob_path_ = persistent_storage_value.get("eblob_path", "").asString();
	eblob_log_path_ = persistent_storage_value.get("eblob_log_path", "").asString();
	eblob_log_flags_ = persistent_storage_value.get("eblob_log_flags", 0).asUInt();
	eblob_sync_interval_ = persistent_storage_value.get("eblob_sync_interval", DEFAULT_EBLOB_SYNC_INTERVAL).asInt();
}

void
configuration::parse_autodiscovery_settings(const Json::Value& config_value) {
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
}

void
configuration::parse_statistics_settings(const Json::Value& config_value) {
	const Json::Value statistics_value = config_value["statistics"];

	is_statistics_enabled_ = statistics_value.get("enabled", false).asBool();
	is_remote_statistics_enabled_ = statistics_value.get("remote_access", false).asBool();
	remote_statistics_port_ = (LT::port)statistics_value.get("remote_port", DEFAULT_STATISTICS_PORT).asUInt();
}

void
configuration::parse_services_settings(const Json::Value& config_value) {
	const Json::Value services_value = config_value["services"];
	service_info_t si;

	for (size_t index = 0; index < services_value.size(); ++index) {
		const Json::Value service_value = services_value[index];
		si.description_ = service_value.get("description", "").asString();
		si.name_ = service_value.get("name", "").asString();
		si.app_name_ = service_value.get("app_name", "").asString();
		si.instance_ = service_value.get("instance", "").asString();
		si.hosts_url_ = service_value.get("hosts_url", "").asString();
		si.control_port_ = service_value.get("control_port", DEFAULT_CONTROL_PORT).asUInt();

		// check values for validity
		if (si.name_.empty()) {
			throw error("service with no name was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
		}

		if (si.app_name_.empty()) {
			throw error("service with no application name was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
		}

		if (si.instance_.empty()) {
			throw error("service with no instance was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
		}

		if (si.hosts_url_.empty()) {
			throw error("service with no hosts_url was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
		}

		if (si.control_port_ == 0) {
			throw error("service with no control port == 0 was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
		}

		// check for duplicate services
		std::map<std::string, service_info_t>::iterator it = services_list_.begin();
		for (;it != services_list_.end(); ++it) {
			if (it->second.name_ == si.name_) {
				throw error("duplicate service with name " + si.name_ + " was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION));
			}
		}

		// no service can have the same app_name + control_port
		it = services_list_.begin();
		for (;it != services_list_.end(); ++it) {
			if (it->second == si) {
				std::string error_msg = "duplicate service with app name " + si.app_name_ + " and ";
				error_msg += "control port " + boost::lexical_cast<std::string>(si.control_port_) + " was found in config! at: " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_msg);
			}
		}

		services_list_[si.name_] = si;
	}
}

void
configuration::load(const std::string& path) {
	boost::mutex::scoped_lock lock(mutex_);

	path_ = path;

	std::ifstream file(path.c_str(), std::ifstream::in);
	
	if (!file.is_open()) {
		throw error("config file: " + path + " failed to open at: " + std::string(BOOST_CURRENT_FUNCTION));
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
		throw error("config file: " + path + " could not be parsed at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
	
	// parse config data
	const Json::Value config_value = root["lsd_config"];
	
	try {
		parse_basic_settings(config_value);
		parse_logger_settings(config_value);
		parse_messages_cache_settings(config_value);
		parse_persistant_storage_settings(config_value);
		parse_autodiscovery_settings(config_value);
		parse_statistics_settings(config_value);
		parse_services_settings(config_value);
	}
	catch (const std::exception& ex) {
		std::string error_msg = "config file: " + path + " could not be parsed. details: ";
		error_msg += ex.what();
		error_msg += " at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
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

size_t
configuration::max_message_cache_size() const {
	return max_message_cache_size_;
}

enum message_cache_type
configuration::message_cache_type() const {
	return message_cache_type_;
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

bool
configuration::is_statistics_enabled() const {
	return is_statistics_enabled_;
}

bool
configuration::is_remote_statistics_enabled() const {
	return is_remote_statistics_enabled_;
}

LT::port
configuration::remote_statistics_port() const {
	return remote_statistics_port_;
}

const std::map<std::string, service_info_t>&
configuration::services_list() const {
	return services_list_;
}

bool
configuration::service_info_by_name(const std::string& name, service_info_t& info) const {
	std::map<std::string, service_info_t>::const_iterator it = services_list_.find(name);
	
	if (it != services_list_.end()) {
		info = it->second;
		return true;
	}

	return false;
}

bool
configuration::service_info_by_name(const std::string& name) const {
	std::map<std::string, service_info_t>::const_iterator it = services_list_.find(name);

	if (it != services_list_.end()) {
		return true;
	}

	return false;
}

std::ostream& operator<<(std::ostream& out, configuration& config) {
	out << "---------- config path: ----------" << config.path_ << "\n";

	// basic
	out << "basic settings\n";
	out << "\tconfig version: " << config.version_ << "\n";
	out << "\tmessage timeout: " << config.message_timeout_ << "\n";
	out << "\tsocket poll timeout: " << config.socket_poll_timeout_ << "\n";
	
	// logger
	out << "logger\n";
	if (config.logger_type_ == STDOUT_LOGGER) {
		out << "\ttype: STDOUT_LOGGER" << "\n";
	}
	else if (config.logger_type_ == FILE_LOGGER) {
		out << "\ttype: FILE_LOGGER" << "\n";
	}
	else if (config.logger_type_ == SYSLOG_LOGGER) {
		out << "\ttype: SYSLOG_LOGGER" << "\n";
	}
	
	if (config.logger_flags_ == PLOG_NONE) {
		out << "\tflags: PLOG_NONE" << "\n";
	}
	else if (config.logger_flags_ == PLOG_ALL) {
		out << "\tflags: PLOG_ALL" << "\n";
	}
	else {
		out << "\tflags: ";
		
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

	out << "\tfile path: " << config.logger_file_path_ << "\n";
 	out << "\tsyslog name: " << config.logger_syslog_name_ << "\n\n";


 	// message cache
 	out << "message cache\n";
 	out << "\tmax ram limit: " << config.max_message_cache_size_ / 1048576 << " Mb\n";

 	if (config.message_cache_type_ == RAM_ONLY) {
 		out << "\ttype: RAM_ONLY\n\n";
 	}
 	else if (config.message_cache_type_ == PERSISTANT) {
 		out << "\ttype: PERSISTANT\n\n";
 	}

 	// persistant storage
 	out << "persistant storage\n";
	out << "\teblob path: " << config.eblob_path_ << "\n";
	out << "\teblob log path: " << config.eblob_log_path_ << "\n";
	out << "\teblob log flags: " << config.eblob_log_flags_ << "\n";
 	out << "\teblob sync interval: " << config.eblob_sync_interval_ << "\n\n";

 	// autodiscovery
 	out << "autodiscovery\n";
	if (config.autodiscovery_type_ == AT_MULTICAST) {
		out << "\ttype: MULTICAST" << "\n";
	}
	else if (config.autodiscovery_type_ == AT_HTTP) {
		out << "\ttype: HTTP" << "\n";
	}

	out << "\tmulticast ip: " << config.multicast_ip_ << "\n";
	out << "\tmulticast port: " << config.multicast_port_ << "\n\n";

	// statistics
	out << "statistics\n";
	if (config.is_statistics_enabled_ == true) {
		out << "\tenabled: true" << "\n";
	}
	else {
		out << "\tenabled: false" << "\n";
	}

	if (config.is_remote_statistics_enabled_ == true) {
		out << "\tremote enabled: true" << "\n";
	}
	else {
		out << "\tremote enabled: false" << "\n";
	}

	out << "\tremote port: " << config.remote_statistics_port_ << "\n\n";

	// services
	out << "services: ";
	std::map<std::string, service_info_t>& sl = config.services_list_;
	
	for (std::map<std::string, service_info_t>::iterator it = sl.begin(); it != sl.end(); ++it) {
		out << "\n\tname: " << it->second.name_ << "\n";
		out << "\tdescription: " << it->second.description_ << "\n";
		out << "\thosts url: " << it->second.hosts_url_ << "\n";
		out << "\tcontrol port: " << it->second.control_port_ << "\n";
	}
	
	return out;
}

} // namespace lsd

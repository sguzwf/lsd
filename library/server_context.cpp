#include "settings.h"

#include "details/server_context.hpp"

namespace pmq {

server_context::server_context(const std::string& config_path) {
	if (config_path.empty()) {
		throw std::runtime_error("config file path is empty string at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	config_.reset(new configuration(config_path));
	
	switch (config_->logger_type()) {
		case STDOUT_LOGGER:
			logger_.reset(new smart_logger<stdout_logger>(config_->logger_flags()));
			break;
			
		case FILE_LOGGER:
			logger_.reset(new smart_logger<file_logger>(config_->logger_flags()));
			((smart_logger<file_logger>*)logger_.get())->init(config_->logger_file_path());
			break;
			
		case SYSLOG_LOGGER:
			logger_.reset(new smart_logger<syslog_logger>(config_->logger_flags()));
			((smart_logger<syslog_logger>*)logger_.get())->init(config_->logger_syslog_name());
			break;
			
		default:
			logger_.reset(new smart_logger<empty_logger>);
			break;
	}
	
	logger_->log("loaded config: %s", config_->config_path().c_str());
	
	//std::cout << *config_ << std::endl;

	zmq_context_.reset(new zmq::context_t(1));
	msg_storage_.reset(new persistent_storage(config_, logger_));
}

server_context::~server_context() {

}

boost::shared_ptr<configuration>
server_context::config() {
	return config_;
}

bool
server_context::validate_service_prefix(const std::string& str) const {
	std::map<std::string, service_info>::const_iterator it = config_->services_list().find(str);
	return (it != config_->services_list().end());
}

boost::shared_ptr<base_logger>
server_context::logger() {
	return logger_;
}

boost::shared_ptr<persistent_storage>
server_context::storage() {
	return msg_storage_;
}

} // namespace pmq
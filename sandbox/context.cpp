#include "settings.h"

#include "context.hpp"

namespace lsd {

context::context(const std::string& config_path) {
	// load configuration from file
	if (config_path.empty()) {
		throw std::runtime_error("config file path is empty string at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	config_.reset(new configuration(config_path));

	// create logger
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
	
	// create zmq context
	zmq_context_.reset(new zmq::context_t(1));

	//std::cout << *config_ << std::endl;
	//msg_storage_.reset(new persistent_storage(config_, logger_));
}

context::~context() {

}

boost::shared_ptr<configuration>
context::config() {
	return config_;
}

boost::shared_ptr<base_logger>
context::logger() {
	return logger_;
}

boost::shared_ptr<zmq::context_t>
context::zmq_context() {
	return zmq_context_;
}

/*
bool
context::validate_service_prefix(const std::string& str) const {
	std::map<std::string, service_info_t>::const_iterator it = config_->services_list().find(str);
	return (it != config_->services_list().end());
}

boost::shared_ptr<persistent_storage>
context::storage() {
	return msg_storage_;
}
*/

} // namespace lsd

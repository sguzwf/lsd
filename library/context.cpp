#include "details/context.hpp"
#include "details/error.hpp"

namespace lsd {

context::context(const std::string& config_path) {
	// load configuration from file
	if (config_path.empty()) {
		throw error("config file path is empty string at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	config_.reset(new configuration(config_path));

	std::cout << *config_ << std::endl;
	exit(0);

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
}

context::~context() {

}

boost::shared_ptr<configuration>
context::config() {
	return config_;
}

boost::shared_ptr<base_logger>
context::logger() {
	boost::mutex::scoped_lock lock(mutex_);
	return logger_;
}

boost::shared_ptr<zmq::context_t>
context::zmq_context() {
	return zmq_context_;
}

} // namespace lsd

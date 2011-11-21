#include "settings.h"

#include "details/client_context.hpp"

namespace pmq {

client_context::client_context(const std::string& service_prefix, const std::string& config_path) {
	if (service_prefix.empty()) {
		throw std::runtime_error("service prefix is empty string at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	if (config_path.empty()) {
		throw std::runtime_error("config file path is empty string at " + std::string(BOOST_CURRENT_FUNCTION));
	}
		
	config_.load(config_path);
	
	switch (config_.logger_type()) {
		case STDOUT_LOGGER:
			logger_.reset(new smart_logger<stdout_logger>(config_.logger_flags()));
			break;
			
		case FILE_LOGGER:
			logger_.reset(new smart_logger<file_logger>(config_.logger_flags()));
			((smart_logger<file_logger>*)logger_.get())->init(config_.logger_file_path());
			break;
			
		case SYSLOG_LOGGER:
			logger_.reset(new smart_logger<syslog_logger>(config_.logger_flags()));
			((smart_logger<syslog_logger>*)logger_.get())->init(config_.logger_syslog_name());
			break;
			
		default:
			logger_.reset(new smart_logger<empty_logger>);
			break;
	}

	logger_->log("loaded config: %s", config_.config_path().c_str());
	
	zmq_context_.reset(new zmq::context_t(1));

	std::map<std::string, service_info> sl = config_.services_list();
	std::map<std::string, service_info>::iterator it = sl.find(service_prefix);
	
	if (it != sl.end()) {
		info_ = it->second;
	}
	else {
		throw std::runtime_error("unknown service prefix " + service_prefix + " at " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

client_context::~client_context() {

}

configuration&
client_context::config() {
	return config_;
}

service_info&
client_context::service_inf() {
	return info_;
}

boost::shared_ptr<base_logger>
client_context::logger() {
	return logger_;
}

} // namespace pmq
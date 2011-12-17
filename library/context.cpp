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

#include "details/context.hpp"
#include "details/error.hpp"

namespace lsd {

context::context(const std::string& config_path) {
	// load configuration from file
	if (config_path.empty()) {
		throw error("config file path is empty string at: " + std::string(BOOST_CURRENT_FUNCTION));
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
	
	logger()->log("loaded config: %s", config()->config_path().c_str());
	//logger()->log(config()->as_string());
	
	// create zmq context
	zmq_context_.reset(new zmq::context_t(1));

	// create statistics collector
	stats_.reset(new statistics_collector(config_, zmq_context_, logger()));
}

context::~context() {
	stats_.reset();
	zmq_context_.reset();
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

boost::shared_ptr<statistics_collector>
context::stats() {
	boost::mutex::scoped_lock lock(mutex_);
	return stats_;
}

} // namespace lsd

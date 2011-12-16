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

#include <cstring>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <zmq.hpp>

#include "details/statistics_collector.hpp"
#include "details/error.hpp"
#include "details/smart_logger.hpp"

namespace lsd {

statistics_collector::statistics_collector(boost::shared_ptr<configuration> config,
										   boost::shared_ptr<zmq::context_t> zmq_context) :
	is_enabled_(false),
	config_(config),
	zmq_context_(zmq_context),
	is_running_(false)
{
	if (!config_) {
		std::string error_str = "configuration object is empty";
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	if (!zmq_context_) {
		std::string error_str = "zmq context object is empty";
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	logger_.reset(new smart_logger<empty_logger>);

	init();
}

statistics_collector::statistics_collector(boost::shared_ptr<configuration> config,
										   boost::shared_ptr<zmq::context_t> zmq_context,
										   boost::shared_ptr<base_logger> logger) :
	is_enabled_(false),
	config_(config),
	zmq_context_(zmq_context),
	is_running_(false)
{
	if (!config_) {
		std::string error_str = "configuration object is empty";
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	if (!zmq_context_) {
		std::string error_str = "zmq context object is empty";
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	set_logger(logger);
	init();
}

statistics_collector::~statistics_collector() {
	is_running_ = false;
	thread_.join();
}

void
statistics_collector::init() {
	is_enabled_ = config_->is_statistics_enabled();

	if (config_->is_remote_statistics_enabled()) {
		// run main thread
		is_running_ = true;
		thread_ = boost::thread(&statistics_collector::process_remote_connection, this);
	}
}

void
statistics_collector::set_logger(boost::shared_ptr<base_logger> logger) {
	if (logger) {
		logger_ = logger;
	}
	else {
		logger_.reset(new smart_logger<empty_logger>);
	}
}

boost::shared_ptr<base_logger>
statistics_collector::logger() {
	boost::mutex::scoped_lock(mutex_);
	return logger_;
}

boost::shared_ptr<configuration>
statistics_collector::config() const {
	return config_;
}

void
statistics_collector::process_remote_connection() {
	// create zmq resp socket
	zmq::socket_t socket(*zmq_context_, ZMQ_REP);
	LT::port port = config()->remote_statistics_port();
	std::string port_str = boost::lexical_cast<std::string>(port);
	socket.bind(("tcp://*:" + port_str).c_str());

	while (is_running_) {
		// poll for request
		zmq_pollitem_t poll_items[1];
		poll_items[0].socket = socket;
		poll_items[0].fd = 0;
		poll_items[0].events = ZMQ_POLLIN;
		poll_items[0].revents = 0;

		int socket_response = zmq_poll(poll_items, 1, 0);
		if (socket_response <= 0) {
			continue;
		}

		// in case we received control message
	    if ((ZMQ_POLLIN & poll_items[0].revents) != ZMQ_POLLIN) {
	    	continue;
	    }

    	zmq::message_t request;

    	// see if we're still running
    	if (!is_running_) {
    		continue;
    	}

    	try {
    		if(!socket.recv(&request)) {
    			logger()->log("recv failed at " + std::string(BOOST_CURRENT_FUNCTION));
    			continue;
    		}
    		else {
				std::string request_str((char*)request.data(), request.size());

				// parse request
				logger()->log("STATS REQUESTED: %s", request_str.c_str());
    		}
    	}
    	catch (const std::exception& ex) {
			std::string error_msg = "some very ugly shit happend while recv on socket at ";
			error_msg += std::string(BOOST_CURRENT_FUNCTION);
			error_msg += " details: " + std::string(ex.what());
			throw error(error_msg);
    	}

    	// see if we're still running
    	if (!is_running_) {
    		continue;
    	}

    	// send response
		try {
			// send response
			std::string stats_json_str = as_json();
			size_t data_len = stats_json_str.length();

			zmq::message_t reply(data_len);
			memcpy((void*)reply.data(), stats_json_str.c_str(), data_len);

			if(!socket.send(reply)) {
				logger()->log("sending failed at " + std::string(BOOST_CURRENT_FUNCTION));
			}
		}
		catch (const std::exception& ex) {
			std::string error_msg = "some very ugly shit happend while send on socket at ";
			error_msg += std::string(BOOST_CURRENT_FUNCTION);
			error_msg += " details: " + std::string(ex.what());
			throw error(error_msg);
		}
	}
}

std::string
statistics_collector::as_json() const {
	Json::FastWriter writer;
	Json::Value root;

	typedef configuration::services_list_t slist_t;
	const slist_t& services = config()->services_list();

	slist_t::const_iterator it = services.begin();
	for (; it != services.end(); ++it) {
		Json::Value service_info;
		service_info["cocaine app"] = it->second.app_name_;
		service_info["description"] = it->second.description_;
		service_info["instance"] = it->second.instance_;
		service_info["hosts_url_"] = it->second.hosts_url_;
		service_info["control_port_"] = it->second.hosts_url_;

		Json::Value service;
		service[it->first] = service_info;
		root["services"] = service;
	}

	return writer.write(root);
}

void
statistics_collector::enable(bool value) {
	is_enabled_ = value;
}

} // namespace lsd

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

#include <stdexcept>

#include <boost/current_function.hpp>

#include "details/client_impl.hpp"
#include "details/http_heartbeats_collector.hpp"
#include "details/error.hpp"
#include "details/cached_message.hpp"

namespace lsd {

client_impl::client_impl(const std::string& config_path) :
	messages_cache_size_(0)
{
	// create lsd context
	std::string ctx_error_msg = "could not create lsd context at: " + std::string(BOOST_CURRENT_FUNCTION) + " ";

	try {
		context_.reset(new lsd::context(config_path));
	}
	catch (const std::exception& ex) {
		throw error(ctx_error_msg + ex.what());
	}

	// create services
	const configuration::services_list_t& services_info_list = config()->services_list();
	configuration::services_list_t::const_iterator it = services_info_list.begin();
	for (; it != services_info_list.end(); ++it) {
		boost::shared_ptr<service_t> service_ptr(new service_t(it->second, context_));
		services_[it->first] = service_ptr;
	}

	logger()->log("client created.");
}

client_impl::~client_impl() {
	disconnect();
	logger()->log("client destroyed.");
}

void
client_impl::connect() {
	boost::mutex::scoped_lock lock(mutex_);
	boost::shared_ptr<configuration> conf;

	if (!context_) {
		std::string error_msg = "lsd context is NULL at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}
	else {
		conf = context_->config();
	}

	if (!conf) {
		std::string error_msg = "configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}

	if (conf->autodiscovery_type() == AT_MULTICAST) {
		// 2 DO
	}
	else if (conf->autodiscovery_type() == AT_HTTP) {
		heartbeats_collector_.reset(new http_heartbeats_collector(conf, context()->zmq_context()));
		heartbeats_collector_->set_callback(boost::bind(&client_impl::service_hosts_pinged_callback, this, _1, _2, _3));
		//heartbeats_collector_->set_logger(logger());
		heartbeats_collector_->run();
	}
}

void
client_impl::disconnect() {
	boost::mutex::scoped_lock lock(mutex_);

	// stop collecting heartbeats
	if (heartbeats_collector_.get()) {
		heartbeats_collector_->stop();
	}
	else {
		std::string error_msg = "empty heartbeats collector object at ";
		error_msg += std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}

	// stop services
	//services_map_t::iterator it = services_.begin();
	//for (; it != services_.end(); ++it) {
		//if(it->second)
	//}
}

void
client_impl::service_hosts_pinged_callback(const service_info_t& s_info,
										   const std::vector<host_info_t>& hosts,
										   const std::vector<handle_info_t>& handles)
{
	// find corresponding service
	services_map_t::iterator it = services_.find(s_info.name_);

	// populate service with pinged hosts and handles
	if (it != services_.end()) {
		if (it->second.get()) {
			it->second->refresh_hosts_and_handles(hosts, handles);
		}
		else {
			std::string error_msg = "empty service object with lsd name " + s_info.name_;
			error_msg += " was found in services. at: " + std::string(BOOST_CURRENT_FUNCTION);
			throw error(error_msg);
		}
	}
	else {
		std::string error_msg = "lsd service with name " + s_info.name_;
		error_msg += " was not found in services. at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}
}

std::string
client_impl::send_message(const void* data,
						  size_t size,
						  const std::string& service_name,
						  const std::string& handle_name)
{
	message_path mpath(service_name, handle_name);
	message_policy mpolicy;
	return send_message(data, size, mpath, mpolicy);
}

std::string
client_impl::send_message(const void* data,
						  size_t size,
						  const message_path& path)
{
	message_policy mpolicy;
	return send_message(data, size, path, mpolicy);
}

std::string
client_impl::send_message(const void* data,
						  size_t size,
						  const message_path& path,
						  const message_policy& policy)
{
	boost::mutex::scoped_lock lock(mutex_);

	// make sure we are not overcapacitated
	size_t message_size = size + sizeof(cached_message) + cached_message::UUID_SIZE + path.container_size();
	size_t new_resulting_size = messages_cache_size() + message_size;

	if (new_resulting_size > config()->max_message_cache_size()) {
		throw error(LSD_MESSAGE_CACHE_OVER_CAPACITY_ERROR, "can not send message, balancer over capacity.");
	}

	// validate message path
	if (!config()->service_info_by_name(path.service_name)) {
		std::string error_str = "message sent to unknown service, check your config file.";
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(LSD_UNKNOWN_SERVICE_ERROR, error_str);
	}

	// find service to send message to
	std::string uuid;
	services_map_t::iterator it = services_.find(path.service_name);

	if (it != services_.end()) {
		boost::shared_ptr<cached_message> msg;
		msg.reset(new cached_message(path, policy, data, size));
		uuid = msg->uuid();

		// send message to handle
		if (it->second) {
			it->second->send_message(msg);
		}
		else {
			std::string error_str = "object for service wth name " + path.service_name;
			error_str += " is emty at " + std::string(BOOST_CURRENT_FUNCTION);
			throw error(error_str);
		}
	}
	else {
		std::string error_str = "no service wth name " + path.service_name;
		error_str += " found at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	update_messages_cache_size();

	// return message uuid
	return uuid;
}

std::string
client_impl::send_message(const std::string& data,
						  const std::string& service_name,
						  const std::string& handle_name)
{
	message_path mpath(service_name, handle_name);
	message_policy mpolicy;
	return send_message(data, mpath, mpolicy);
}

std::string
client_impl::send_message(const std::string& data,
						  const message_path& path)
{
	message_policy mpolicy;
	return send_message(data, path, mpolicy);
}

std::string
client_impl::send_message(const std::string& data,
						  const message_path& path,
						  const message_policy& policy)
{
	return send_message(data.c_str(), data.length(), path, policy);
}

void
client_impl::set_response_callback(boost::function<void(const std::string&, void* data, size_t size)> callback)
{
	response_callback_ = callback;
}

boost::shared_ptr<context>
client_impl::context() {
	if (!context_.get()) {
		throw error("lsd context object is empty at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return context_;
}

boost::shared_ptr<base_logger>
client_impl::logger() {
	return context()->logger();
}

size_t
client_impl::messages_cache_size() const {
	return messages_cache_size_;
}

void
client_impl::update_messages_cache_size() {
	// collect cache sizes of all services
	messages_cache_size_ = 0;

	services_map_t::iterator it = services_.begin();
	for (; it != services_.end(); ++it) {

		boost::shared_ptr<service_t> service_ptr = it->second;
		if (service_ptr) {
			messages_cache_size_ += service_ptr->cache_size();
		}
		else {
			std::string error_str = "object for service wth name " + it->first;
			error_str += " is empty at " + std::string(BOOST_CURRENT_FUNCTION);
			throw error(error_str);
		}
	}

	// total size of all queues in all services
	context()->stats()->update_used_cache_size(messages_cache_size_);
}

boost::shared_ptr<configuration>
client_impl::config() {
	boost::shared_ptr<configuration> conf = context()->config();
	if (!conf.get()) {
		throw error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return conf;
}

} // namespace lsd

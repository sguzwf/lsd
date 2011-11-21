#include "settings.h"

#include <stdexcept>

#include <boost/current_function.hpp>

#include "client_impl.hpp"
#include "http_heartbeats_collector.hpp"

namespace lsd {

client_impl::client_impl(const std::string& config_path) :
	is_running_(false)
{
	// create lsd context
	std::string ctx_error_msg = "could not create lsd context at: " + std::string(BOOST_CURRENT_FUNCTION) + " ";

	try {
		context_.reset(new context(config_path));
	}
	catch (const std::exception& ex) {
		throw std::runtime_error(ctx_error_msg + ex.what());
	}

	if (!context_.get()) {
		throw std::runtime_error(ctx_error_msg);
	}

	// get config
	boost::shared_ptr<configuration> conf = context_->config();
	if (!conf.get()) {
		throw std::runtime_error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	// create services
	const configuration::services_list_t& services_info_list = conf->services_list();
	configuration::services_list_t::const_iterator it = services_info_list.begin();
	for (; it != services_info_list.end(); ++it) {
		boost::shared_ptr<service_t> service_ptr(new service_t(it->second, context_->zmq_context()));
		service_ptr->set_logger(context_->logger());
		services_[it->first] = service_ptr;
	}

	logger()->log("client created.");
}

client_impl::~client_impl() {
	//heartbeats_collector_.reset(NULL);
	
	//is_running_ = false;
	//thread_.join();

	//updated_groups_.clear();
}

void
client_impl::connect() {
	boost::shared_ptr<configuration> conf;

	if (!context_.get()) {
		std::string error_msg = "lsd context is NULL at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw std::runtime_error(error_msg);
	}
	else {
		conf = context_->config();
	}

	if (!conf.get()) {
		std::string error_msg = "configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw std::runtime_error(error_msg);
	}

	if (conf->autodiscovery_type() == AT_MULTICAST) {
		// 2 DO
	}
	else if (conf->autodiscovery_type() == AT_HTTP) {
		heartbeats_collector_.reset(new http_heartbeats_collector(conf, context_->zmq_context()));
		heartbeats_collector_->set_callback(boost::bind(&client_impl::service_hosts_pinged_callback, this, _1, _2, _3));
		heartbeats_collector_->set_logger(context_->logger());
		heartbeats_collector_->run();
	}

	//is_running_ = true;
	//thread_ = boost::thread(&server_impl::process_sockets, this);
}

void
client_impl::disconnect() {

}

void
client_impl::service_hosts_pinged_callback(const service_info_t& s_info, const std::vector<host_info_t>& hosts, const std::vector<handle_info_t>& handles) {
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
			throw std::runtime_error(error_msg);
		}
	}
	else {
		std::string error_msg = "lsd service with name " + s_info.name_;
		error_msg += " was not found in services. at: " + std::string(BOOST_CURRENT_FUNCTION);
		throw std::runtime_error(error_msg);
	}
}

/*
void
server_impl::process_sockets() {
	while (is_running_) {
		if (!updated_groups_.empty()) {
			logger()->log(PLOG_DEBUG, "updated_groups: %d", updated_groups_.size());
			
			// get updated groups
			std::vector<std::string> groups;
			
			boost::mutex mutex;
			{
				boost::mutex::scoped_lock lock(mutex);
				std::swap(updated_groups_, groups);
			}

			// update server sockets
			for (size_t i = 0; i < groups.size(); ++i) {
				if (heartbeats_collector_.get()) {
					std::vector<host_info> hosts;// = heartbeats_collector_->get_hosts_by_group(groups[i]);
					
					// find existing socket
					server_sockets_map::iterator it = server_sockets_.find(groups[i]);
					
					if (!hosts.empty()) {
						if (it != server_sockets_.end()) {
							logger()->log("update hosts for group: %s", groups[i].c_str());

							if (NULL == it->second.get()) {
								std::string error_msg = "server socket ptr for hosts group \"";
								error_msg += groups[i] + "\" is NULL! method - server::process_sockets()";
								throw std::runtime_error(error_msg);
							}

							it->second->update_socket_hosts(hosts);
							it->second->wake_up();
						}
						else {
							logger()->log("create socket for group: %s", groups[i].c_str());

							// if there exists such service in config, create new server socket and start it
							service_inf si;
							bool service_info_retrieved = true;
							try {
								si =context_->config()->service_by_prefix(groups[i]);
							}
							catch (...) {
								service_info_retrieved = false;
							}
							
							if (service_info_retrieved) {
								boost::shared_ptr<server_socket> ss_ptr;
								ss_ptr.reset(new server_socket(si, hosts, context_));
								server_sockets_[groups[i]] = ss_ptr;
								ss_ptr->wake_up();
							}
						}
					}
					else {
						if (it != server_sockets_.end()) {
							logger()->log("kill socket for group: %s", groups[i].c_str());
							it->second.reset();
							server_sockets_.erase(it);
						}
					}

					logger()->log(PLOG_DEBUG, "server sockets count: %d", server_sockets_.size());
				}
			}
		}
	}
}

void
server_impl::clients_group_changed(const std::string& group_name) {
	updated_groups_.push_back(group_name);
}

void
server_impl::send_message(const std::string& msg) {
	// get a list of all services, send to all
	std::map<std::string, service_inf>::const_iterator it = context_->config()->services_list().begin();
	for (; it != context_->config()->services_list().end(); ++it) {
		//context_->storage()->add_message(msg, it->second.prefix_);
	}
}

void
server_impl::send_message(const std::string& msg, const std::vector<std::string>& services) {
	if (services.empty()) {
		return;
	}

	for (size_t i = 0; i < services.size(); ++i) {
		if (context_->validate_service_prefix(services[i])) {
			context_->storage()->add_message(msg, services[i]);
		}
	}
}

void
server_impl::send_message(const std::string& msg, const std::string& service_prefix) {
	// drop messages sent to unknown services
	if (!context_->validate_service_prefix(service_prefix)) {
		return;
	}

	context_->storage()->add_message(msg, service_prefix);

	// wake up corresponding socket
	server_sockets_map::iterator it = server_sockets_.find(service_prefix);
	if (it != server_sockets_.end()) {
		it->second->wake_up();
	}
}
*/

boost::shared_ptr<base_logger>
client_impl::logger() {
	return context_->logger();
}

} // namespace lsd

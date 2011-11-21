#include "settings.h"

#include "details/server_impl.hpp"

#include <map>

namespace pmq {

server_impl::server_impl(const std::string& config_path) :
	is_running_(false)
{
	context_.reset(new server_context(config_path));
	logger()->log("server created.");
}

server_impl::~server_impl() {
	heartbeats_collector_.reset(NULL);
	
	is_running_ = false;
	thread_.join();

	updated_groups_.clear();
}

void
server_impl::connect() {
	boost::shared_ptr<configuration> conf = context_->config();
			
	if (conf->autodiscovery_type() == AT_MULTICAST) {
		heartbeats_collector_.reset(new multicast_heartbeats_collector(conf->multicast_ip(), conf->multicast_port()));
		heartbeats_collector_->set_callback(boost::bind(&server_impl::clients_group_changed, this, _1));
		heartbeats_collector_->set_logger(context_->logger());
		heartbeats_collector_->run();
	}
	else if (conf->autodiscovery_type() == AT_CONDUCTOR) {
		heartbeats_collector_.reset(new conductor_heartbeats_collector(conf->multicast_ip(), conf->multicast_port()));
		heartbeats_collector_->set_callback(boost::bind(&server_impl::clients_group_changed, this, _1));
		heartbeats_collector_->set_logger(context_->logger());
		heartbeats_collector_->run();
	}
		
	is_running_ = true;
	thread_ = boost::thread(&server_impl::process_sockets, this);
}

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
					std::vector<host_info> hosts = heartbeats_collector_->get_hosts_by_group(groups[i]);
					
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
							service_info si;
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
	std::map<std::string, service_info>::const_iterator it = context_->config()->services_list().begin();
	for (; it != context_->config()->services_list().end(); ++it) {
		context_->storage()->add_message(msg, it->second.prefix_);
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

boost::shared_ptr<base_logger>
server_impl::logger() {
	return context_->logger();
}

} // namespace pmq

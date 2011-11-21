#include "settings.h"

#include "multicast_heartbeats_collector.hpp"
#include "helpers.hpp"
#include "client.hpp"
#include "structs.hpp"

#include <boost/tokenizer.hpp>

namespace lsd {

multicast_heartbeats_collector::multicast_heartbeats_collector(const std::string& mip, unsigned short mport) :
	multicast_ip_(mip),
	multicast_port_(mport),
	is_stopping_(false)
{
	logger_.reset(new base_logger);
}

multicast_heartbeats_collector::~multicast_heartbeats_collector() {
	is_stopping_ = true;
	
	refresher_.reset(NULL);
	mcast_server_->stop();
	mcast_server_.reset(NULL);
}

void
multicast_heartbeats_collector::collect_heartbeat(std::string heartbeat) {	
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(":");
	tokenizer tokens(heartbeat, sep);
	
	int count = 0;
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
		count++;
	}
	
	// check arity
	if (count == 3) {
		tokenizer::iterator tok_iter = tokens.begin();
		std::string group = *tok_iter;
		std::string hostname = *(++tok_iter);
		std::string ip = *(++tok_iter);
		
		std::pair<std::string, host_heartbeat> item = std::make_pair(group, host_heartbeat(ip, hostname));

		bool inserted = false;
		
		boost::mutex mutex;		
		{
			boost::mutex::scoped_lock lock(mutex);

			if (clients_.find(group) == clients_.end()) {
				clients_.insert(item);
				inserted = true;
			}
			else {
				bool found = false;
				std::pair<clients_map_iter, clients_map_iter> range = clients_.equal_range(group);
				for (clients_map_iter it = range.first; it != range.second; ++it) {
					if (it->second == item.second) {
						found = true;
						it->second.last_heartbeat_time_ = time(NULL); // update heartbeat
					}
				}

				if (!found) {
					clients_.insert(item);
					inserted = true;
				}
			}
			
			if (inserted) {
				// invoke callback with group name
				if (callback_) {
					callback_(item.first);
				}
			}
		}
	}
}

void
multicast_heartbeats_collector::output_clients_info() {
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	logger_->log("------------ clients info ------------\n");
	for (clients_map_citer it = clients_.begin(); it != clients_.end(); ++it) {
		logger_->log("%s %s (%s)", it->first.c_str(), it->second.host_.ip_.c_str(), it->second.host_.name_.c_str());
	}
}

void
multicast_heartbeats_collector::set_logger(boost::shared_ptr<base_logger> logger) {
	logger_ = logger;
}

void
multicast_heartbeats_collector::clean_up_dead_hosts() {	
	time_t now = time(NULL);
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);
	
	for (clients_map_iter it = clients_.begin(); it != clients_.end();) {
		time_t last_heartbeat_time = it->second.last_heartbeat_time_;
		if (now - last_heartbeat_time > (HEARTBEAT_INTERVAL * 2)) {
			std::string group = it->first;
			clients_.erase(it++);
			
			if (callback_) {
				callback_(group);
			}
		}
		else {
			++it;
		}
	}
}

void
multicast_heartbeats_collector::refresh_hostname() {
	get_hostname_and_ip(hostname_, ip_);
}

std::multimap<std::string, host_heartbeat>
multicast_heartbeats_collector::get_all_hosts() const {
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);
	
	return clients_;
}

void
multicast_heartbeats_collector::set_callback(boost::function<void(const std::string&)> f) {
	callback_ = f;
}

std::vector<host_info>
multicast_heartbeats_collector::get_hosts_by_group(const std::string& group) const {
	boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	std::vector<host_info> retval;
		
	clients_map_citer it = clients_.find(group);
	
	if (it == clients_.end()) {
		return retval;
	}

	std::pair<clients_map_citer, clients_map_citer> range = clients_.equal_range(group);
	for (clients_map_citer it = range.first; it != range.second; ++it) {
		retval.push_back(it->second.host_);
	}
	
	return retval;
}

void	
multicast_heartbeats_collector::run() {
	refresh_hostname();
	refresher_.reset(new refresher(boost::bind(&multicast_heartbeats_collector::clean_up_dead_hosts, this), HEARTBEAT_INTERVAL));
	
	mcast_server_.reset(new multicast_server(boost::bind(&multicast_heartbeats_collector::collect_heartbeat, this, _1)));
	mcast_server_->set_iface_ip(ip_.c_str());
	mcast_server_->set_multicast_ip(multicast_ip_.c_str());
	mcast_server_->set_port(multicast_port_);

	mcast_server_->run();
}
	
} // namespace lsd
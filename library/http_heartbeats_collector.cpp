#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include "json/json.h"
#include "details/progress_timer.hpp"
#include "details/http_heartbeats_collector.hpp"

namespace lsd {

http_heartbeats_collector::http_heartbeats_collector(boost::shared_ptr<configuration> config, boost::shared_ptr<zmq::context_t> zmq_context) :
	config_(config), zmq_context_(zmq_context)
{
	logger_.reset(new base_logger);
}

http_heartbeats_collector::~http_heartbeats_collector() {
	stop();
}

void
http_heartbeats_collector::run() {
	boost::mutex::scoped_lock lock(mutex_);

	// create http hosts fetchers
	const std::map<std::string, service_info_t>& services_list = config_->services_list();
	std::map<std::string, service_info_t>::const_iterator it = services_list.begin();
	
	for (; it != services_list.end(); ++it) {
		boost::shared_ptr<curl_hosts_fetcher> fetcher;
		fetcher.reset(new curl_hosts_fetcher(it->second.hosts_url_, curl_fetcher_timeout, it->second));
		fetcher->set_callback(boost::bind(&http_heartbeats_collector::hosts_callback, this, _1, _2));
		
		hosts_fetchers_.push_back(fetcher);
	}

	// create hosts pinger
	refresher_.reset(new refresher(boost::bind(&http_heartbeats_collector::services_ping_callback, this), hosts_ping_timeout));
}

void
http_heartbeats_collector::stop() {
	logger_->log("STOP");
	boost::mutex::scoped_lock lock(mutex_);

	// kill http hosts fetchers	
	for (size_t i = 0; i < hosts_fetchers_.size(); ++i) {
		hosts_fetchers_[i].reset();
	}

	// kill hosts pinger
	refresher_.reset();
}

void
http_heartbeats_collector::set_callback(heartbeats_collector::callback_t callback) {
	boost::mutex::scoped_lock lock(mutex_);
	callback_ = callback;
}

void
http_heartbeats_collector::hosts_callback(std::vector<host_info_t>& hosts, service_info_t s_info) {
	logger_->log("received hosts from fetcher for service: " + s_info.name_);

	boost::mutex::scoped_lock lock(mutex_);
	fetched_services_hosts_[s_info.name_] = hosts;
}

void
http_heartbeats_collector::services_ping_callback() {
	try {
		service_hosts_map services_2_ping;

		{
			boost::mutex::scoped_lock lock(mutex_);
			services_2_ping.insert(fetched_services_hosts_.begin(), fetched_services_hosts_.end());
		}

		const std::map<std::string, service_info_t>& services_list = config_->services_list();

		for (service_hosts_map::iterator it = services_2_ping.begin(); it != services_2_ping.end(); ++it) {
			std::map<std::string, service_info_t>::const_iterator sit = services_list.find(it->first);

			if (sit != services_list.end()) {
				const service_info_t& s_info = sit->second;
				std::vector<host_info_t>& hosts = it->second;
				ping_service_hosts(s_info, hosts);
			}
		}
	}
	catch (const std::exception& ex) {
		std::string error_msg = "something ugly happened with heartbeats collector at %s";
		logger_->log(error_msg.c_str(), std::string(BOOST_CURRENT_FUNCTION).c_str());
	}
}

bool
http_heartbeats_collector::get_metainfo_from_host(const service_info_t& s_info,
												  LT::ip_addr ip,
												  std::string& response)
{
	// create req socket
	std::auto_ptr<zmq::socket_t> zmq_socket;
	zmq_socket.reset(new zmq::socket_t(*(zmq_context_), ZMQ_REQ));
	std::string ex_err;

	// connect to host
	std::string host_ip_str = host_info_t::string_from_ip(ip);
	std::string connection_str = "tcp://" + host_ip_str + ":";
	connection_str += boost::lexical_cast<std::string>(s_info.control_port_);

	int timeout = 100;
	zmq_socket->setsockopt(ZMQ_LINGER, &timeout, sizeof(timeout));
	zmq_socket->connect(connection_str.c_str());

	// send request for cocaine metadata
	Json::Value msg(Json::objectValue);
	Json::FastWriter writer;

	msg["version"] = 2;
	msg["action"] = "info";

	std::string info_request = writer.write(msg);
	zmq::message_t message(info_request.length());
	memcpy((void *)message.data(), info_request.c_str(), info_request.length());

	bool sent_request_ok = true;

	try {
		sent_request_ok = zmq_socket->send(message);
	}
	catch (const std::exception& ex) {
		sent_request_ok = false;
		ex_err = ex.what();
	}

	if (!sent_request_ok) {
		// in case of bad send
		std::string error_msg = "could not send metadata request for lsd app: " + s_info.name_;
		error_msg += ", host: " + host_ip_str + " at " + std::string(BOOST_CURRENT_FUNCTION);
		logger_->log(PLOG_ERROR, error_msg + ex_err);

		return false;
	}

	// create polling structure
	zmq_pollitem_t poll_items[1];
	poll_items[0].socket = *zmq_socket;
	poll_items[0].fd = 0;
	poll_items[0].events = ZMQ_POLLIN;
	poll_items[0].revents = 0;

	// poll for responce
	progress_timer timer;
	int res = -1;

	while (res <= 0) {
		res = zmq_poll(poll_items, 1, DEFAULT_SOCKET_POLL_TIMEOUT);
		if (timer.elapsed().milliseconds() > DEFAULT_SOCKET_PING_TIMEOUT) {
			break;
		}
	}

	if (res <= 0) {
		return false;
	}

	if ((ZMQ_POLLIN & poll_items[0].revents) != ZMQ_POLLIN) {
		return false;
	}

	// receive cocaine control data
	zmq::message_t reply;
	bool received_response_ok = true;

	try {
		received_response_ok = zmq_socket->recv(&reply);
		response = std::string(static_cast<char*>(reply.data()), reply.size());
	}
	catch (const std::exception& ex) {
		received_response_ok = false;
		ex_err = ex.what();
	}

	if (!received_response_ok) {
		// in case of bad recv
		std::string error_msg = "counl not receive metadata response for lsd app: " + s_info.name_;
		error_msg += ", host: " + host_ip_str + " at " + std::string(BOOST_CURRENT_FUNCTION);
		logger_->log(PLOG_ERROR, error_msg + ex_err);

		return false;
	}

	return true;
}

void
http_heartbeats_collector::ping_service_hosts(const service_info_t& s_info, std::vector<host_info_t>& hosts) {
	logger_->log("pinging hosts from for service: " + s_info.name_);

	boost::mutex::scoped_lock lock(mutex_);

	std::vector<host_info_t> responded_hosts;
	std::vector<handle_info_t> collected_handles;
	std::multimap<LT::ip_addr, handle_info_t> hosts_and_handles;

	for (size_t i = 0; i < hosts.size(); ++i) {

		// request host metadata
		std::string metadata;
		if (!get_metainfo_from_host(s_info, hosts[i].ip_, metadata)) {
			continue;
		}

		// collect service handles info from host responce
		std::vector<handle_info_t> host_handles;

		try {
			parse_host_response(s_info, hosts[i].ip_, metadata, host_handles);
		}
		catch (const std::exception& ex) {
			// in case of unparsealbe response, skip
			std::string error_msg = "heartbeat response parsing error for lsd app: " + s_info.name_;
			error_msg += ", host: " + host_info_t::string_from_ip(hosts[i].ip_);
			error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION) + " details: ";
			logger_->log(PLOG_ERROR, error_msg + ex.what());

			continue;
		}

		// if we found valid lsd handles at host
		if (!host_handles.empty()) {

			// add properly pinged and alive host
			responded_hosts.push_back(hosts[i]);

			// add host handles
			for (size_t j = 0; j < host_handles.size(); ++j) {

				// cache host handle for checking later
				std::pair<LT::ip_addr, handle_info_t> p = std::make_pair(hosts[i].ip_, host_handles[j]);
				hosts_and_handles.insert(p);

				// check if such handle already exists on the list
				bool found = false;
				for (size_t k = 0; k < collected_handles.size(); ++k) {
					if (collected_handles[k] == host_handles[j]) {
						found = true;
					}
				}

				// add only new handles
				if (!found) {
					collected_handles.push_back(host_handles[j]);
				}
			}
		}
	}

	// check that all handles from pinged hosts are the same
	logger_->log(PLOG_DEBUG, "--- validating hosts handles ---");
	validate_host_handles(s_info, responded_hosts, hosts_and_handles);

	// pass collected data to callback
	logger_->log("CALL");
	callback_(s_info, responded_hosts, collected_handles);
}

void
http_heartbeats_collector::validate_host_handles(const service_info_t& s_info,
												 const std::vector<host_info_t>& hosts,
												 const std::multimap<LT::ip_addr, handle_info_t>& hosts_and_handles) const
{
	// check that all hosts have the same callback
	if (hosts.empty()) {
		return;
	}

	bool outstanding_handles = false;

	// iterate thought responded hosts
	for (size_t i = 0; i < hosts.size() - 1; ++i) {
		// get host handles from map
		LT::ip_addr ip1 = hosts[i].ip_;
		LT::ip_addr ip2 = hosts[i + 1].ip_;

		std::multimap<LT::ip_addr, handle_info_t>::const_iterator it1, end1, it2, end2;
		boost::tie(it1, end1) = hosts_and_handles.equal_range(ip1);
		boost::tie(it2, end2) = hosts_and_handles.equal_range(ip2);

		if (it1 == hosts_and_handles.end()) {
			// host not found in map — error!
			std::string err_msg = "host ip 1: " + host_info_t::string_from_ip(ip1);
			err_msg += " was not found in hosts_and_handles map";
			err_msg += " for lsd app " + s_info.name_ + " at " + std::string(BOOST_CURRENT_FUNCTION);
			logger_->log(PLOG_ERROR, err_msg);
			outstanding_handles = true;
		}
		else if (it2 == hosts_and_handles.end()) {
			// host not found in map — error!
			std::string err_msg = "host ip 2: " + host_info_t::string_from_ip(ip2);
			err_msg += " was not found in hosts_and_handles map";
			err_msg += " for lsd app " + s_info.name_ + " at " + std::string(BOOST_CURRENT_FUNCTION);
			logger_->log(PLOG_ERROR, err_msg);
			outstanding_handles = true;
		}
		else {
			// find all handles from host 1 in host 2
			for (; it1 != end1; ++it1) {
				bool found = false;
				for (; it2 != end2; ++it2) {
					if (it1->second == it2->second) {
						found = true;
					}
				}

				if (!found) {
					// log error
					std::ostringstream handle_stream;
					handle_stream << it1->second;

					std::string err_msg = "handle (" + handle_stream.str() + ") from host " + host_info_t::string_from_ip(ip1);
					err_msg += " was not found in handles of host " + host_info_t::string_from_ip(ip2);
					err_msg += " for lsd app " + s_info.name_ + " at " + std::string(BOOST_CURRENT_FUNCTION);
					logger_->log(PLOG_ERROR, err_msg);
					outstanding_handles = true;
				}
			}

			boost::tie(it1, end1) = hosts_and_handles.equal_range(ip1);
			boost::tie(it2, end2) = hosts_and_handles.equal_range(ip2);

			// find all handles from host 2 in host 1
			for (; it2 != end2; ++it2) {
				bool found = false;
				for (; it1 != end1; ++it1) {
					if (it2->second == it1->second) {
						found = true;
					}
				}

				if (!found) {
					// log error
					std::ostringstream handle_stream;
					handle_stream << it2->second;

					std::string err_msg = "handle (" + handle_stream.str() + ") from host " + host_info_t::string_from_ip(ip2);
					err_msg += " was not found in handles of host " + host_info_t::string_from_ip(ip1);
					err_msg += " for lsd app " + s_info.name_ + " at " + std::string(BOOST_CURRENT_FUNCTION);
					logger_->log(PLOG_ERROR, err_msg);
					outstanding_handles = true;
				}
			}
		}
	}

	if (!outstanding_handles) {
		logger_->log(PLOG_DEBUG, "no outstanding handles for any host");
	}
}

void
http_heartbeats_collector::parse_host_response(const service_info_t& s_info,
											   LT::ip_addr ip,
											   const std::string& response,
											   std::vector<handle_info_t>& handles)
{
	Json::Value root;
	Json::Reader reader;
	bool parsing_successful = reader.parse(response, root);
	std::string host_ip = host_info_t::string_from_ip(ip);

	std::string host_info_err = "server (" + host_ip + "), app " + s_info.app_name_;

	if (!parsing_successful) {
		std::string err_msg = "server metadata response could not be parsed for ";
		err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw std::runtime_error(err_msg);
	}
	
	const Json::Value apps = root["apps"];

	if (!apps.isObject() || !apps.size()) {
		std::string err_msg = "no apps found in server metadata response for ";
		err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw std::runtime_error(err_msg);
	}

	// iterate throuhg the apps
    Json::Value::Members app_names(apps.getMemberNames());

    for (Json::Value::Members::iterator nm_it = app_names.begin(); nm_it != app_names.end(); ++nm_it) {
        // get the app name and app manifest
        std::string parsed_app_name(*nm_it);

        // if this is the app we're pinging
        if (parsed_app_name != s_info.app_name_) {
        	continue;
        }

        // is app running?
        bool is_app_running = false;
		Json::Value app(apps[s_info.app_name_]);

		if (app.isObject()) {
			is_app_running = app.get("running", false).asBool();
		}
		else {
			std::string err_msg = "server metadata response has bad json structure for ";
			err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
			logger_->log(PLOG_ERROR, err_msg);
			continue;
		}

        if (!is_app_running) {
        	std::string err_msg = "server is not running for ";
        	err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
			logger_->log(PLOG_ERROR, err_msg);
			continue;
        }

    	//iterate through app handles
    	Json::Value app_tasks(apps[s_info.app_name_]["tasks"]);
    	if (!app_tasks.isObject() || !app_tasks.size()) {
        	std::string err_msg = "no existing handles found for ";
        	err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
			throw std::runtime_error(err_msg);
		}

    	Json::Value::Members handles_names(app_tasks.getMemberNames());
    		
    	for (Json::Value::Members::iterator hn_it = handles_names.begin(); hn_it != handles_names.end(); ++hn_it) {
    		std::string handle_name(*hn_it);
    		Json::Value handle(app_tasks[handle_name]);

    		if (!handle.isObject() || !handle.size()) {
    			std::string err_msg = "error while parsing handle " + handle_name + " for " ;
    			err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
				throw std::runtime_error(err_msg);
			}

			// get handle type
    		std::string handle_type = handle.get("type", "").asString();
    		if (handle_type != "server+lsd") {
    			continue;
    		}

			// parse lsd handle
			std::string endpoint = handle.get("endpoint", "").asString();
			std::string route = handle.get("route", "").asString();
			std::string instance = "";
			lsd_types::port port = 0;

			size_t found = route.find_first_of("/");

			if (found != std::string::npos) {
				instance = route.substr(0, found);
			}

			found = endpoint.find_first_of(":");
			if (found != std::string::npos) {
				std::string port_str = endpoint.substr(found + 1, endpoint.length() - found);

				try {
					port = boost::lexical_cast<lsd_types::port>(port_str);
				}
				catch(...) {
				}
			}

			handle_info_t s_handle(handle_name, s_info.name_, port);

			bool handle_ok = true;

			// instance empty?
			if (instance.empty()) {
				handle_ok = false;
				std::string err_msg = "error while parsing handle " + handle_name;
				err_msg += ", instance is empty string for ";
				err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
				logger_->log(PLOG_ERROR, err_msg);
			}

			// not our service instance?
			if (instance != s_info.instance_) {
				handle_ok = false;
			}

			// port undefined?
			if (s_handle.port_ == 0) {
				handle_ok = false;
				std::string err_msg = "error while parsing handle " + handle_name;
				err_msg += ", handle port is zero for ";
				err_msg += host_info_err + " at " + std::string(BOOST_CURRENT_FUNCTION);
				logger_->log(PLOG_ERROR, err_msg);
			}
				
			// add parsed and validated handle to list
			if (handle_ok) {
				handles.push_back(s_handle);
			}
        }
    }
}

void
http_heartbeats_collector::set_logger(boost::shared_ptr<base_logger> logger) {
	boost::mutex::scoped_lock lock(mutex_);
	logger_ = logger;
}

} // namespace lsd

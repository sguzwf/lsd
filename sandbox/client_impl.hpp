#ifndef _LSD_CLIENT_IMPL_HPP_INCLUDED_
#define _LSD_CLIENT_IMPL_HPP_INCLUDED_

#include <vector>
#include <map>
#include <memory>
#include <zmq.hpp>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include "heartbeats_collector.hpp"

#include "context.hpp"

namespace lsd {

class client_impl : private boost::noncopyable {
public:
	client_impl(const std::string& config_path = "");
	virtual ~client_impl();

	void connect();
	void disconnect();
	void send_message(const std::string& msg, const std::string& service_name, const std::string& handle_name) {};
	
	boost::shared_ptr<base_logger> logger();
	
private:
	void service_hosts_pinged_callback(const service_info_t& s_info, const std::vector<host_info_t>& hosts, const std::vector<handle_info_t>& handles);
	//void process_sockets() {};
	//void clients_group_changed(const std::string& group_name) {};

private:
	typedef std::map<std::string, boost::shared_ptr<service_t> > services_map_t;

private:
	// main lsd context
	boost::shared_ptr<context> context_;

	// lsd service name mapped to service
	services_map_t services_;

	// heartsbeat collector
	std::auto_ptr<heartbeats_collector> heartbeats_collector_;

	// threading primitives
	boost::thread thread_;
	boost::mutex mutex_;
	volatile bool is_running_;
};

} // namespace lsd

#endif // _LSD_CLIENT_IMPL_HPP_INCLUDED_

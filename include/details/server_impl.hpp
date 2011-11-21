#ifndef _PMQ_SERVER_IMPL_HPP_INCLUDED_
#define _PMQ_SERVER_IMPL_HPP_INCLUDED_

#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <zmq.hpp>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include "heartbeats_collector.hpp"
#include "multicast_heartbeats_collector.hpp"
#include "conductor_heartbeats_collector.hpp"

#include "server_socket.hpp"
#include "structs.hpp"
#include "server_context.hpp"

namespace pmq {

class server_impl : private boost::noncopyable {
public:
	server_impl(const std::string& config_path = "");
	virtual ~server_impl();

	void connect();
	
	void send_message(const std::string& msg);
	void send_message(const std::string& msg, const std::vector<std::string>& services);
	void send_message(const std::string& msg, const std::string& service_prefix);
	
	boost::shared_ptr<base_logger> logger();
	
private:
	void process_sockets();
	void clients_group_changed(const std::string& group_name);

private:
	typedef std::map<std::string, boost::shared_ptr<server_socket> > server_sockets_map;
	
	std::auto_ptr<heartbeats_collector> heartbeats_collector_;
	
	std::vector<std::string> updated_groups_;
	server_sockets_map server_sockets_;

	boost::thread thread_;
	boost::mutex mutex_;
	volatile bool is_running_;

	boost::shared_ptr<server_context> context_;
};

} // namespace pmq

#endif // _PMQ_SERVER_IMPL_HPP_INCLUDED_
#ifndef _PMQ_SERVER_SOCKET_HPP_INCLUDED_
#define _PMQ_SERVER_SOCKET_HPP_INCLUDED_

#include <vector>
#include <map>
#include <memory>	
#include <zmq.hpp>

#include <boost/thread/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>

#include "persistent_storage.hpp"
#include "structs.hpp"
#include "server_context.hpp"

namespace pmq {

class server_socket  {
public:
	server_socket(const service_info& sinfo, const std::vector<host_info>& hosts, boost::shared_ptr<server_context> context);
	virtual ~server_socket();

	void update_socket_hosts(const std::vector<host_info>& hosts);
	void output_hosts();
	
	void wake_up();

private:
	void connect();
	void disconnect();
	void process_message_queue();
	void connect_new_hosts(const std::vector<host_info>& hosts);
	boost::shared_ptr<base_logger> logger();

private:
	service_info service_info_;
	std::vector<host_info> hosts_;
	boost::shared_ptr<server_context> context_;
	
	std::map<std::string, cached_message> sent_messages_; // message uid, cached message
	
	std::auto_ptr<zmq::socket_t> zmq_control_socket_;
	bool receiving_control_socket_ok_;
	
	boost::thread thread_;
	boost::mutex mutex_;
	volatile bool is_running_;
};

} // namespace pmq

#endif // _PMQ_SERVER_SOCKET_HPP_INCLUDED_
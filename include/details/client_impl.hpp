#ifndef _PMQ_CLIENT_IMPL_HPP_INCLUDED_
#define _PMQ_CLIENT_IMPL_HPP_INCLUDED_

#include <memory>
#include <zmq.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include "heartbeat_notifier.hpp"
#include "client_context.hpp"
#include "smart_logger.hpp"

namespace pmq {

class client_impl : private boost::noncopyable  {

public:
	client_impl(const std::string& service_prefix, const std::string& config_path);
	virtual ~client_impl();
	
	void connect();

private:
	void run();
	void configure_heartbeat(const std::string& service_id, const std::string& ip, unsigned short port);
	boost::shared_ptr<base_logger> logger();
	
private:
	boost::shared_ptr<client_context> context_;

	std::auto_ptr<pmq::heartbeat_notifier> heartbeat_notifier_;
    std::auto_ptr<zmq::socket_t> zmq_socket_;
	
	boost::thread thread_;
	boost::condition condition_;
	boost::mutex mutex_;
	
	volatile bool is_running_;
};

} // namespace pmq

#endif // _PMQ_CLIENT_IMPL_HPP_INCLUDED_
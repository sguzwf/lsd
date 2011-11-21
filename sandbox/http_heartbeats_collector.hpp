#ifndef _LSD_HTTP_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
#define _LSD_HTTP_HEARTBEATS_COLLECTOR_HPP_INCLUDED_

#include <memory>
#include <string>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include <zmq.hpp>

#include "multicast_server.hpp"
#include "refresher.hpp"
#include "smart_logger.hpp"
#include "heartbeats_collector.hpp"
#include "curl_hosts_fetcher.hpp"
#include "globals.hpp"
#include "configuration.hpp"

namespace lsd {
	
class http_heartbeats_collector : public heartbeats_collector, private boost::noncopyable {
public:
	http_heartbeats_collector(boost::shared_ptr<configuration> config, boost::shared_ptr<zmq::context_t> zmq_context);
	virtual ~http_heartbeats_collector();

	void run();
	void stop();

	void set_logger(boost::shared_ptr<base_logger> logger);
	void set_callback(heartbeats_collector::callback_t callback);
	
private:
	void hosts_callback(std::vector<lsd::host_info_t>& hosts, service_info_t tag);
	void services_ping_callback();
	void ping_service_hosts(const service_info_t& s_info, std::vector<host_info_t>& hosts);

	void parse_host_response(const std::string& cocaine_app_name,
							 const std::string& response,
							 const std::string& host_ip,
							 std::vector<handle_info_t>& handles);

	void validate_host_handles(const service_info_t& s_info,
							   const std::vector<host_info_t>& hosts,
							   const std::multimap<LT::ip_addr, handle_info_t>& hosts_and_handles) const;

private:
	boost::shared_ptr<configuration> config_;
	boost::shared_ptr<zmq::context_t> zmq_context_;
	boost::shared_ptr<base_logger> logger_;

	typedef std::map<std::string, std::vector<host_info_t> > service_hosts_map;

	std::vector<boost::shared_ptr<curl_hosts_fetcher> > hosts_fetchers_;
	service_hosts_map fetched_services_hosts_;
	std::auto_ptr<refresher> refresher_;

	heartbeats_collector::callback_t callback_;
};

} // namespace lsd

#endif // _LSD_HTTP_HEARTBEATS_COLLECTOR_HPP_INCLUDED_

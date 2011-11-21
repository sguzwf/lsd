#ifndef _PMQ_MULTICAST_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
#define _PMQ_MULTICAST_HEARTBEATS_COLLECTOR_HPP_INCLUDED_

#include <memory>
#include <string>
#include <stdexcept>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include "multicast_server.hpp"
#include "refresher.hpp"
#include "structs.hpp"
#include "smart_logger.hpp"
#include "heartbeats_collector.hpp"

namespace pmq {
	
class multicast_heartbeats_collector : public heartbeats_collector, private boost::noncopyable {
public:
	multicast_heartbeats_collector(const std::string& mip, unsigned short mport);
	virtual ~multicast_heartbeats_collector();

	void run();
	
	// functor is called when the list of hosts had been changed
	void set_callback(boost::function<void(const std::string&)> f);

	std::multimap<std::string, host_heartbeat> get_all_hosts() const;
	std::vector<host_info> get_hosts_by_group(const std::string& group) const;

	void output_clients_info();
	void set_logger(boost::shared_ptr<base_logger> logger);

private:
	typedef std::multimap<std::string, host_heartbeat>::iterator clients_map_iter;
	typedef std::multimap<std::string, host_heartbeat>::const_iterator clients_map_citer;
	
private:
	void collect_heartbeat(std::string heartbeat);
	void refresh_hostname();
	void clean_up_dead_hosts();

private:
	boost::shared_ptr<base_logger> logger_;
	
	std::auto_ptr<refresher> refresher_;
	std::auto_ptr<multicast_server> mcast_server_;
	
	std::string multicast_ip_;
	unsigned short multicast_port_;
	
	std::string ip_;
	std::string hostname_;

	std::multimap<std::string, host_heartbeat> clients_;
	boost::function<void(const std::string&)> callback_;
	bool is_stopping_;
};

} // namespace pmq

#endif // _PMQ_MULTICAST_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
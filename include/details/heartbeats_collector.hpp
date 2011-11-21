#ifndef _PMQ_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
#define _PMQ_HEARTBEATS_COLLECTOR_HPP_INCLUDED_

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include "smart_logger.hpp"
#include "structs.hpp"

namespace pmq {

class heartbeats_collector {
public:
	virtual void run() = 0;
	virtual void set_callback(boost::function<void(const std::string&)> f) = 0;
	virtual std::multimap<std::string, host_heartbeat> get_all_hosts() const = 0;
	virtual std::vector<host_info> get_hosts_by_group(const std::string& group) const = 0;
	virtual void output_clients_info() = 0;
	virtual void set_logger(boost::shared_ptr<base_logger> logger) = 0;
};

} // namespace pmq

#endif // _PMQ_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
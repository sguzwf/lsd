#ifndef _LSD_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
#define _LSD_HEARTBEATS_COLLECTOR_HPP_INCLUDED_

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include "smart_logger.hpp"
#include "globals.hpp"
#include "configuration.hpp"

namespace lsd {

class heartbeats_collector {
public:
	typedef boost::function<void(const service_info_t&, const std::vector<host_info_t>&, const std::vector<handle_info_t>&)> callback_t;

	virtual void run() = 0;
	virtual void stop() = 0;
	virtual void set_callback(callback_t callback) = 0;
	virtual void set_logger(boost::shared_ptr<base_logger> logger) = 0;
};

} // namespace lsd

#endif // _LSD_HEARTBEATS_COLLECTOR_HPP_INCLUDED_
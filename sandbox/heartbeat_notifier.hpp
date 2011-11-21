#ifndef _LSD_HEARTBEAT_NOTIFIER_HPP_INCLUDED_
#define _LSD_HEARTBEAT_NOTIFIER_HPP_INCLUDED_

#include <memory>
#include <string>

#include <boost/shared_ptr.hpp>

#include "multicast_client.hpp"
#include "refresher.hpp"
#include "smart_logger.hpp"

namespace lsd {

class heartbeat_notifier : private boost::noncopyable {
public:
	heartbeat_notifier();
	heartbeat_notifier(const std::string& mip, unsigned short mport);
	heartbeat_notifier(const std::string& gid, const std::string& mip, unsigned short mport);
	virtual ~heartbeat_notifier();

	void set_logger(boost::shared_ptr<base_logger> logger);

private:
	void run(boost::uint32_t interval);
	void send_heartbeat();
	void refresh_hostname();
	void configure();

private:
	std::auto_ptr<refresher> refresher_;
	multicast_client mcast_client_;
	
	std::string multicast_ip_;
	unsigned short multicast_port_;
	
	std::string hostname_;
	std::string ip_;
	std::string group_id_;
	
	boost::shared_ptr<base_logger> logger_;
};

} // namespace lsd

#endif // _LSD_HEARTBEAT_NOTIFIER_HPP_INCLUDED_
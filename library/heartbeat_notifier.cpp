#include "settings.h"

#include "details/heartbeat_notifier.hpp"
#include "details/helpers.hpp"
#include "details/structs.hpp"

namespace pmq {

heartbeat_notifier::heartbeat_notifier() {
	logger_.reset(new base_logger);
}

heartbeat_notifier::heartbeat_notifier(const std::string& mip, unsigned short mport) :
	multicast_ip_(mip),
	multicast_port_(mport),
	group_id_("undefined_group") {
	logger_.reset(new base_logger);
	run(HEARTBEAT_INTERVAL);
}
	
heartbeat_notifier::heartbeat_notifier(const std::string& gid, const std::string& mip, unsigned short mport) :
	multicast_ip_(mip),
	multicast_port_(mport),
	group_id_(gid) {
	logger_.reset(new base_logger);
	run(HEARTBEAT_INTERVAL);
}

heartbeat_notifier::~heartbeat_notifier() {
	refresher_.reset(NULL);
	mcast_client_.stop();
}

void
heartbeat_notifier::set_logger(boost::shared_ptr<base_logger> logger) {
	logger_ = logger;
}

void
heartbeat_notifier::send_heartbeat() {
	refresh_hostname();

	if (!mcast_client_.is_running()) {
		mcast_client_.run();
	}
	
	logger_->log(std::string("heartbeat: ") + group_id_ + ":" + hostname_ + ":" + ip_);
	mcast_client_.send_message(group_id_ + ":" + hostname_ + ":" + ip_);
}

void
heartbeat_notifier::refresh_hostname() {
	get_hostname_and_ip(hostname_, ip_);
}

void
heartbeat_notifier::run(boost::uint32_t interval) {
	refresh_hostname();

	mcast_client_.set_iface_ip(ip_.c_str());
	mcast_client_.set_multicast_ip(multicast_ip_.c_str());
	mcast_client_.set_port(multicast_port_);

	mcast_client_.run();

	refresher_.reset(new refresher(boost::bind(&heartbeat_notifier::send_heartbeat, this), interval));
}
	
} // namespace pmq
#ifndef _LSD_SERVICE_HPP_INCLUDED_
#define _LSD_SERVICE_HPP_INCLUDED_

#include <string>
#include <map>
#include <vector>

#include <zmq.hpp>
#include <boost/shared_ptr.hpp>

#include "service_info.hpp"
#include "host_info.hpp"
#include "handle_info.hpp"
#include "handle.hpp"
#include "smart_logger.hpp"

namespace lsd {

// predeclarations
template <typename LSD_T> class service;
template <typename LSD_T> std::ostream& operator << (std::ostream& out, const service<LSD_T>& s);

template <typename LSD_T>
class service {
public:
	typedef std::map<typename LSD_T::ip_addr, std::string> hosts_map_t;
	typedef std::map<std::string, handle<LSD_T> > handles_map_t;

public:
	service(const service_info<LSD_T>& info, boost::shared_ptr<zmq::context_t> zmq_context);

	void refresh_hosts_and_handles(const std::vector<host_info<LSD_T> >& hosts,
								   const std::vector<handle_info<LSD_T> >& handles);

	boost::shared_ptr<base_logger> logger();
	void set_logger(boost::shared_ptr<base_logger> logger);

public:
	template <typename T> friend std::ostream& operator << (std::ostream& out, const service<T>& s);

private:
	service_info<LSD_T> info_;
	hosts_map_t hosts_;		// hosts (ip, hostname)
	handles_map_t handles_;	// collected from hosts data (handle name, port)
	boost::shared_ptr<zmq::context_t> zmq_context_;
	boost::shared_ptr<base_logger> logger_;
};

template <typename LSD_T>
service<LSD_T>::service(const service_info<LSD_T>& info, boost::shared_ptr<zmq::context_t> zmq_context) :
	info_(info),
	zmq_context_(zmq_context)
{
	logger_.reset(new base_logger);
}

template <typename LSD_T> boost::shared_ptr<base_logger>
service<LSD_T>::logger() {
	return logger_;
}

template <typename LSD_T> void
service<LSD_T>::set_logger(boost::shared_ptr<base_logger> logger) {
	logger_ = logger;
}

template <typename LSD_T> void
service<LSD_T>::refresh_hosts_and_handles(const std::vector<host_info<LSD_T> >& hosts, const std::vector<handle_info<LSD_T> >& handles) {
	std::cout << "service " << info_.name_ << " refreshed with:\n";

	std::cout << "hosts: \n";
	for (size_t i = 0; i < hosts.size(); ++i) {
		std::cout << hosts[i] << std::endl;
	}

	std::cout << "handles: \n";
	for (size_t i = 0; i < handles.size(); ++i) {
		std::cout << handles[i] << std::endl;
	}

	// store hosts
	// create handles.
}

template <typename LSD_T>
std::ostream& operator << (std::ostream& out, const service<LSD_T>& s) {
	out << "----- service info: -----\n";
	out << s.info_;

	return out;
};



} // namespace lsd

#endif // _LSD_SERVICE_HPP_INCLUDED_

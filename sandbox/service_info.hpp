#ifndef _LSD_SERVICE_INFO_HPP_INCLUDED_
#define _LSD_SERVICE_INFO_HPP_INCLUDED_

#include <string>
#include <map>

#include "structs.hpp"

namespace lsd {

// predeclaration
template <typename LSD_T> class service_info;
typedef service_info<LT> service_info_t;

template <typename LSD_T>
class service_info {
public:	
	service_info() {};
	service_info(const service_info<LSD_T>& info) {
		this->name_ = info.name_;
		this->description_ = info.description_;
		this->app_name_ = info.app_name_;
		this->instance_ = info.instance_;
		this->hosts_url_ = info.hosts_url_;
		this->control_port_ = info.control_port_;
	};

	service_info (const std::string& name,
				  const std::string& description,
				  const std::string& app_name,
				  const std::string& instance,
				  const std::string& hosts_url) :
					  name_(name),
					  description_(description),
					  app_name_(app_name),
					  instance_(instance),
					  hosts_url_(hosts_url),
					  control_port_(DEFAULT_CONTROL_PORT) {};
	
	bool operator == (const service_info& si) {
		return (name_ == si.name_ &&
				hosts_url_ == si.hosts_url_ &&
				instance_ == si.instance_ &&
				control_port_ == si.control_port_);
	};

	// config-defined data
	std::string name_;
	std::string description_;
	std::string app_name_;
	std::string instance_;
	std::string hosts_url_;
	typename LSD_T::port control_port_;
};

template <typename LSD_T>
std::ostream& operator << (std::ostream& out, const service_info<LSD_T>& service_inf) {
	out << "lsd service name: " << service_inf.name_ << "\n";
	out << "description: " << service_inf.description_ << "\n";
	out << "app name: " << service_inf.app_name_ << "\n";
	out << "instance: " << service_inf.instance_ << "\n";
	out << "hosts url: " << service_inf.hosts_url_ << "\n";
	out << "control port: " << service_inf.control_port_ << "\n";

	return out;
};


} // namespace lsd

#endif // _LSD_SERVICE_INFO_HPP_INCLUDED_

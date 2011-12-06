#ifndef _LSD_HANDLE_INFO_HPP_INCLUDED_
#define _LSD_HANDLE_INFO_HPP_INCLUDED_

#include <string>
#include <map>

#include "structs.hpp"

namespace lsd {

// predeclaration
template <typename LSD_T> class handle_info;
typedef handle_info<LT> handle_info_t;

template <typename LSD_T>
class handle_info {
public:
	handle_info() {};

	handle_info(const std::string& name,
				const std::string& service_name,
				typename LSD_T::port port) :
				name_(name),
				service_name_(service_name),
				port_(port) {};

	handle_info(const handle_info<LSD_T>& h_info) :
	name_(h_info.name_),
	service_name_(h_info.service_name_),
	port_(h_info.port_) {};

	bool operator == (const handle_info<LSD_T>& sh) const {
		return (name_ == sh.name_ &&
				service_name_ == sh.service_name_ &&
				port_ == sh.port_);
	};

	std::string name_;
	std::string service_name_;
	typename LSD_T::port port_;
};

template <typename LSD_T>
std::ostream& operator << (std::ostream& out, const handle_info<LSD_T>& handle) {
	out << "service name: " << handle.service_name_ << " name: " << handle.name_ << ", port: " << handle.port_;

	return out;
};

} // namespace lsd

#endif // _LSD_HANDLE_INFO_HPP_INCLUDED_

#ifndef _LSD_HANDLE_INFO_HPP_INCLUDED_
#define _LSD_HANDLE_INFO_HPP_INCLUDED_

#include <string>
#include <map>

namespace lsd {

template <typename LSD_T>
class handle_info {
public:
	handle_info() {};

	handle_info(const std::string& name, std::string instance, typename LSD_T::port port) :
	name_(name), instance_(instance), port_(port) {};

	handle_info(const handle_info<LSD_T>& h_info) :
	name_(h_info.name_), instance_(h_info.instance_), port_(h_info.port_) {};

	bool operator == (const handle_info<LSD_T>& sh) const {
		return (name_ == sh.name_ &&
				instance_ == sh.instance_ &&
				port_ == sh.port_);
	};

	std::string name_;
	std::string instance_;
	typename LSD_T::port port_;
};

template <typename LSD_T>
std::ostream& operator << (std::ostream& out, const handle_info<LSD_T>& handle) {
	out << "name: " << handle.name_ << ", instance: " << handle.instance_;
	out << ", port: " << handle.port_;

	return out;
};

} // namespace lsd

#endif // _LSD_HANDLE_INFO_HPP_INCLUDED_

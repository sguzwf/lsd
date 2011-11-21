#ifndef _LSD_CLIENT_HPP_INCLUDED_
#define _LSD_CLIENT_HPP_INCLUDED_

#include <memory>
#include <string>

#include <boost/utility.hpp>

#include "client_forward.hpp"

namespace lsd {

class client : private boost::noncopyable {
public:
	client(const std::string& config_path = "");
	virtual ~client();

	void connect();
	void disconnect();

	// send message to specific application and specific handle
	void send_message(const std::string& msg, const std::string& service_name, const std::string& handle_name);
	
private:
	std::auto_ptr<client_impl> impl_;
};

} // namespace lsd

#endif // _LSD_CLIENT_HPP_INCLUDED_

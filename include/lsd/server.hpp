#ifndef _PMQ_SERVER_HPP_INCLUDED_
#define _PMQ_SERVER_HPP_INCLUDED_

#include <memory>
#include <vector>
#include <string>
#include <map>

#include <boost/utility.hpp>

#include "server_forward.hpp"

namespace pmq {

class server : private boost::noncopyable {
public:
	server(const std::string& config_path = "");
	virtual ~server();

	void connect();
	
	void send_message(const std::string& msg); // mesg to all services in config
	void send_message(const std::string& msg, const std::vector<std::string>& services); // to list of services
	void send_message(const std::string& msg, const std::string& service_prefix); // to specific service
	
private:
	std::auto_ptr<server_impl> impl_;
};

} // namespace pmq

#endif // _PMQ_SERVER_HPP_INCLUDED_
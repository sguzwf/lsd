#ifndef _PMQ_CLIENT_HPP_INCLUDED_
#define _PMQ_CLIENT_HPP_INCLUDED_

#include <memory>

#include <boost/utility.hpp>

#include <pmq/client_forward.hpp>

namespace pmq {

class client : private boost::noncopyable  {
public:
	client(const std::string& service_prefix, const std::string& config_path);
	virtual ~client();
	
	void connect();
	
private:
	std::auto_ptr<client_impl> impl_;
};

} // namespace pmq

#endif // _PMQ_CLIENT_HPP_INCLUDED_
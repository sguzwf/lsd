#ifndef _PQM_CLIENT_CONTEXT_HPP_INCLUDED_
#define _PQM_CLIENT_CONTEXT_HPP_INCLUDED_

#include <string>
#include <map>
#include <memory>
#include <stdexcept>

#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/current_function.hpp>

#include "smart_logger.hpp"
#include "structs.hpp"
#include "configuration.hpp"

namespace pmq {

class client_context : private boost::noncopyable {
public:
	client_context(const std::string& service_prefix, const std::string& config_path);
	virtual ~client_context();

	configuration& config();
	service_info& service_inf();
	boost::shared_ptr<base_logger> logger();

	std::auto_ptr<zmq::context_t> zmq_context_;

private:
	boost::shared_ptr<base_logger> logger_;
	configuration config_;
	service_info info_;
};

} // namespace pmq

#endif // _PQM_CLIENT_CONTEXT_HPP_INCLUDED_
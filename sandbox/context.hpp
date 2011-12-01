#ifndef _LSD_SERVER_CONTEXT_HPP_INCLUDED_
#define _LSD_SERVER_CONTEXT_HPP_INCLUDED_

#include <string>
#include <map>
#include <memory>
#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "smart_logger.hpp"
#include "configuration.hpp"

namespace lsd {

class context : private boost::noncopyable {
public:
	context(const std::string& config_path = "");
	virtual ~context();

	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<zmq::context_t> zmq_context();

private:
	boost::shared_ptr<zmq::context_t> zmq_context_;
	boost::shared_ptr<base_logger> logger_;
	boost::shared_ptr<configuration> config_;
};

} // namespace lsd

#endif // _LSD_SERVER_CONTEXT_HPP_INCLUDED_

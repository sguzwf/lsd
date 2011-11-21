#ifndef _LSD_SERVER_CONTEXT_HPP_INCLUDED_
#define _LSD_SERVER_CONTEXT_HPP_INCLUDED_

#include <string>
#include <map>
#include <memory>
#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "smart_logger.hpp"
#include "structs.hpp"
#include "configuration.hpp"

//#include "persistent_storage.hpp"

namespace lsd {

class context : private boost::noncopyable {
public:
	context(const std::string& config_path = "");
	virtual ~context();

	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<zmq::context_t> zmq_context();

	/*
	bool validate_service_prefix(const std::string& str) const;
	boost::shared_ptr<persistent_storage> storage();
	*/

private:
	boost::shared_ptr<zmq::context_t> zmq_context_;
	boost::shared_ptr<base_logger> logger_;
	boost::shared_ptr<configuration> config_;

	//boost::shared_ptr<persistent_storage> msg_storage_;
};

} // namespace lsd

#endif // _LSD_SERVER_CONTEXT_HPP_INCLUDED_

#ifndef _PQM_SERVER_CONTEXT_HPP_INCLUDED_
#define _PQM_SERVER_CONTEXT_HPP_INCLUDED_

#include <string>
#include <map>
#include <memory>
#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "smart_logger.hpp"
#include "persistent_storage.hpp"
#include "structs.hpp"
#include "configuration.hpp"

namespace pmq {

class server_context : private boost::noncopyable {
public:
	server_context(const std::string& config_path = "");
	virtual ~server_context();

	bool validate_service_prefix(const std::string& str) const;
	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<persistent_storage> storage();

	std::auto_ptr<zmq::context_t> zmq_context_;

private:
	boost::shared_ptr<persistent_storage> msg_storage_;
	boost::shared_ptr<base_logger> logger_;
	boost::shared_ptr<configuration> config_;
};

} // namespace pmq

#endif // _PQM_SERVER_CONTEXT_HPP_INCLUDED_
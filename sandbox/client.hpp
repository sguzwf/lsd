#ifndef _LSD_CLIENT_HPP_INCLUDED_
#define _LSD_CLIENT_HPP_INCLUDED_

#include <string>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "forwards.hpp"
#include "structs.hpp"

namespace lsd {

class client : private boost::noncopyable {
public:
	client(const std::string& config_path = "");
	virtual ~client();

	void connect();
	void disconnect();

	std::string send_message(const void* data,
							 size_t size,
							 const std::string& service_name,
							 const std::string& handle_name);

	std::string send_message(const void* data,
							 size_t size,
							 const message_path& path);

	std::string send_message(const void* data,
							 size_t size,
							 const message_path& path,
							 const message_policy& policy);

	std::string send_message(const std::string& data,
							 const std::string& service_name,
							 const std::string& handle_name);

	std::string send_message(const std::string& data,
							 const message_path& path);

	std::string send_message(const std::string& data,
							 const message_path& path,
							 const message_policy& policy);

	void set_response_callback(boost::function<void(const std::string&, void* data, size_t size)> callback);

private:
	boost::shared_ptr<client_impl> get_impl();

	boost::shared_ptr<client_impl> impl_;
	mutable boost::mutex mutex_;
};

} // namespace lsd

#endif // _LSD_CLIENT_HPP_INCLUDED_

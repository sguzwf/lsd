#include <stdexcept>

#include <boost/current_function.hpp>

#include "lsd/client.hpp"
#include "details/client_impl.hpp"

namespace lsd {

client::client(const std::string& config_path) {
	impl_.reset(new client_impl(config_path));
}

client::~client() {
}

void
client::connect() {
	get_impl()->connect();
}

void
client::disconnect() {
	get_impl()->disconnect();
}

std::string
client::send_message(const void* data,
					 size_t size,
					 const std::string& service_name,
					 const std::string& handle_name)
{
	return get_impl()->send_message(data, size, service_name, handle_name);
}

std::string
client::send_message(const void* data,
					 size_t size,
					 const message_path& path)
{
	return get_impl()->send_message(data, size, path);
}

std::string
client::send_message(const void* data,
					 size_t size,
					 const message_path& path,
					 const message_policy& policy)
{
	return get_impl()->send_message(data, size, path, policy);
}

std::string
client::send_message(const std::string& data,
					 const std::string& service_name,
					 const std::string& handle_name)
{
	return get_impl()->send_message(data, service_name, handle_name);
}

std::string
client::send_message(const std::string& data,
					 const message_path& path)
{
	return get_impl()->send_message(data, path);
}

std::string
client::send_message(const std::string& data,
					 const message_path& path,
					 const message_policy& policy)
{
	return get_impl()->send_message(data, path, policy);
}

void
client::set_response_callback(boost::function<void(const std::string&, void* data, size_t size)> callback) {
	get_impl()->set_response_callback(callback);
}

inline boost::shared_ptr<client_impl>
client::get_impl() {
	boost::mutex::scoped_lock lock(mutex_);

	if (impl_.get()) {
		return impl_;
	}
	else {
		throw error("client_impl object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

} // namespace lsd

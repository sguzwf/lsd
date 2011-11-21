#include "settings.h"

#include <stdexcept>

#include <boost/current_function.hpp>

#include "client.hpp"
#include "client_impl.hpp"

namespace lsd {

client::client(const std::string& config_path) {
	impl_.reset(new client_impl(config_path));
}

client::~client() {
}

void
client::connect() {
	if (impl_.get()) {
		impl_->connect();
	}
	else {
		throw std::runtime_error("client_impl object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

void client::disconnect() {
	if (impl_.get()) {
		impl_->disconnect();
	}
	else {
		throw std::runtime_error("client_impl object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

void
client::send_message(const std::string& msg, const std::string& service_name, const std::string& handle_name) {
	if (impl_.get()) {
		impl_->send_message(msg, service_name, handle_name);
	}
	else {
		throw std::runtime_error("client_impl object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

} // namespace lsd

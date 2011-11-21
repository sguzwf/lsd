#include "settings.h"

#include "pmq/server.hpp"
#include "details/server_impl.hpp"

namespace pmq {

server::server(const std::string& config_path)
{
	impl_.reset(new server_impl(config_path));
}

server::~server() {
}

void
server::connect() {
	impl_->connect();
}

void
server::send_message(const std::string& msg) {
	impl_->send_message(msg);
}

void
server::send_message(const std::string& msg, const std::vector<std::string>& services) {
	impl_->send_message(msg, services);
}

void
server::send_message(const std::string& msg, const std::string& service_prefix) {
	impl_->send_message(msg, service_prefix);
}

} // namespace pmq
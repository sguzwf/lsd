#include "settings.h"

#include <stdexcept>
#include <boost/current_function.hpp>

#include "pmq/client.hpp"
#include "details/client_impl.hpp"

namespace pmq {

client::client(const std::string& service_prefix, const std::string& config_path) {
	impl_.reset(new client_impl(service_prefix, config_path));
	
	if (!impl_.get()) {
		throw std::runtime_error("no implementation at " + std::string(BOOST_CURRENT_FUNCTION));
	}
}

void
client::connect() {			
	if (impl_.get()) {
		impl_->connect();
	}
}

client::~client() {
}

} // namespace pmq
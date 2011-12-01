#include "settings.h"

#include <algorithm>
#include <stdexcept>
#include <uuid/uuid.h>
#include <map>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/progress.hpp>

#include "message_cache.hpp"
#include "error.hpp"

namespace lsd {

message_cache::message_cache(boost::shared_ptr<lsd::context> context,
							 enum message_cache_type type) :
	context_(context),
	type_(type)
{

}

message_cache::~message_cache() {
}

boost::shared_ptr<lsd::context>
message_cache::context() {
	return context_;
}

void
message_cache::enqueue(boost::shared_ptr<cached_message> message) {
	new_messages_.push_back(message);
}

size_t
message_cache::new_messages_count() const {
	return new_messages_.size();
}

size_t
message_cache::sent_messages_count() const {
	return sent_messages_.size();
}

boost::shared_ptr<cached_message>
message_cache::get_new_message() const {
	return new_messages_.front();
}

boost::shared_ptr<cached_message>
message_cache::get_sent_message(const std::string& uuid) const {
	messages_index_t::const_iterator it = sent_messages_.find(uuid);

	if (it == sent_messages_.end()) {
		std::string error_str = "can not find message with uuid " + uuid;
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	if (!it->second.get()) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return it->second;
}

void
message_cache::move_new_message_to_sent() {
	boost::shared_ptr<cached_message> msg = new_messages_.front();

	if (!msg.get()) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	sent_messages_[msg->uuid()] = msg;
	new_messages_.pop_front();
}

void
message_cache::move_sent_message_to_new(const std::string& uuid) {
	messages_index_t::iterator it = sent_messages_.find(uuid);

	if (it == sent_messages_.end()) {
		std::string error_str = "can not find message with uuid " + uuid;
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	if (!it->second.get()) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	new_messages_.push_back(it->second);
	sent_messages_.erase(it);
}

void
message_cache::remove_message_from_cache(const std::string& uuid) {
	messages_index_t::iterator it = sent_messages_.find(uuid);

	if (it == sent_messages_.end()) {
		std::string error_str = "can not find message with uuid " + uuid;
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	sent_messages_.erase(it);
}

void
message_cache::make_all_messages_new() {
	messages_index_t::iterator it = sent_messages_.begin();
	for (; it != sent_messages_.end(); ++it) {
		if (!it->second.get()) {
			throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
		}

		new_messages_.push_back(it->second);
	}

	sent_messages_.clear();
}

} // namespace lsd

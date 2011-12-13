#include <algorithm>
#include <stdexcept>
#include <uuid/uuid.h>
#include <map>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/progress.hpp>

#include "details/error.hpp"
#include "details/message_cache.hpp"
#include "details/progress_timer.hpp"

namespace lsd {

message_cache::message_cache(boost::shared_ptr<lsd::context> context,
							 enum message_cache_type type) :
	context_(context),
	type_(type)
{
	new_messages_.reset(new message_queue_t);
}

message_cache::~message_cache() {
}

boost::shared_ptr<lsd::context>
message_cache::context() {
	return context_;
}

boost::shared_ptr<base_logger>
message_cache::logger() {
	return context()->logger();
}

boost::shared_ptr<configuration>
message_cache::config() {
	boost::shared_ptr<configuration> conf = context()->config();
	if (!conf.get()) {
		throw error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return conf;
}

std::deque<boost::shared_ptr<cached_message> >&
message_cache::new_messages() {
	if (!new_messages_) {
		std::string error_str = "new messages queue object is empty at ";
		error_str += std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	return *new_messages_;
}

void
message_cache::enqueue(boost::shared_ptr<cached_message> message) {
	boost::mutex::scoped_lock lock(mutex_);
	new_messages().push_back(message);
}

void
message_cache::append_message_queue(message_queue_ptr_t queue) {
	boost::mutex::scoped_lock lock(mutex_);

	// validate new queue
	if (!queue || queue->empty()) {
		return;
	}

	// append messages
	new_messages().insert(new_messages_->end(), queue->begin(), queue->end());
}

cached_message&
message_cache::get_new_message() {
	boost::mutex::scoped_lock lock(mutex_);

	// validate message
	if (!new_messages().front()) {
		std::string error_str = "empty message object at ";
		error_str += std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	return *(new_messages().front());
}

size_t
message_cache::new_messages_count() {
	boost::mutex::scoped_lock lock(mutex_);
	return new_messages().size();
}

size_t
message_cache::sent_messages_count() {
	boost::mutex::scoped_lock lock(mutex_);
	return sent_messages_.size();
}

cached_message&
message_cache::get_sent_message(const std::string& uuid) {
	boost::mutex::scoped_lock lock(mutex_);
	messages_index_t::const_iterator it = sent_messages_.find(uuid);

	if (it == sent_messages_.end()) {
		std::string error_str = "can not find message with uuid " + uuid;
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	if (!it->second) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return *(it->second);
}

void
message_cache::move_new_message_to_sent() {
	boost::mutex::scoped_lock lock(mutex_);
	boost::shared_ptr<cached_message> msg = new_messages().front();

	if (!msg) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	sent_messages_.insert(std::make_pair(msg->uuid(), msg));
	new_messages().pop_front();
}

void
message_cache::move_sent_message_to_new(const std::string& uuid) {
	boost::mutex::scoped_lock lock(mutex_);
	messages_index_t::iterator it = sent_messages_.find(uuid);

	if (it == sent_messages_.end()) {
		return;
		//std::string error_str = "can not find message with uuid " + uuid;
		//error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		//throw error(error_str);
	}

	boost::shared_ptr<cached_message> msg = it->second;

	if (!msg) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	sent_messages_.erase(it);

	msg->mark_as_unsent();
	new_messages().push_back(msg);
}

void
message_cache::remove_message_from_cache(const std::string& uuid) {
	boost::mutex::scoped_lock lock(mutex_);
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
	boost::mutex::scoped_lock lock(mutex_);
	messages_index_t::iterator it = sent_messages_.begin();
	for (; it != sent_messages_.end(); ++it) {
		if (!it->second) {
			throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
		}

		new_messages().push_back(it->second);
	}

	sent_messages_.clear();
}

void
message_cache::process_timed_out_messages() {
	boost::mutex::scoped_lock lock(mutex_);

	int timedout_count = 0;

	messages_index_t::iterator it = sent_messages_.begin();
	for (; it != sent_messages_.end(); ++it) {

		// get single sent message
		boost::shared_ptr<cached_message> msg = it->second;
		if (!msg) {
			throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
		}

		// check whether it timed out
		bool is_timed_out = false;
		const timeval& timestamp = msg->sent_timestamp();

		if (timestamp.tv_sec != 0 &&
			timestamp.tv_usec != 0 &&
			msg->policy().deadline > 0.0)
		{
			if (progress_timer::elapsed_from_time(&timestamp) > msg->policy().deadline) {
				is_timed_out = true;
			}
		}

		if (is_timed_out) {
			// move message to new
			msg->mark_as_unsent();
			new_messages().push_back(msg);

			// remove from sent messages
			sent_messages_.erase(it++);
			++timedout_count;
		}
		else {
			++it;
		}
	}
}

} // namespace lsd

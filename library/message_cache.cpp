//
// Copyright (C) 2011 Rim Zaidullin <creator@bash.org.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <algorithm>
#include <stdexcept>
#include <uuid/uuid.h>
#include <map>
#include <cstring>
#include <algorithm>

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

boost::shared_ptr<std::deque<boost::shared_ptr<cached_message> > >
message_cache::new_messages() {
	logger()->log("message_cache::new_messages");

	if (!new_messages_) {
		std::string error_str = "new messages queue object is empty at ";
		error_str += std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	return new_messages_;
}

void
message_cache::enqueue(boost::shared_ptr<cached_message> message) {
	boost::mutex::scoped_lock lock(mutex_);
	new_messages_->push_back(message);
}

void
message_cache::append_message_queue(message_queue_ptr_t queue) {
	boost::mutex::scoped_lock lock(mutex_);

	// validate new queue
	if (!queue || queue->empty()) {
		return;
	}

	// append messages
	new_messages_->insert(new_messages_->end(), queue->begin(), queue->end());
}

cached_message&
message_cache::get_new_message() {
	boost::mutex::scoped_lock lock(mutex_);

	// validate message
	if (!new_messages_->front()) {
		std::string error_str = "empty message object at ";
		error_str += std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	return *(new_messages_->front());
}

size_t
message_cache::new_messages_count() {
	boost::mutex::scoped_lock lock(mutex_);
	return new_messages_->size();
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
	boost::shared_ptr<cached_message> msg = new_messages_->front();

	if (!msg) {
		throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	sent_messages_.insert(std::make_pair(msg->uuid(), msg));
	new_messages_->pop_front();
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

	msg->mark_as_sent(false);
	new_messages_->push_back(msg);
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

		new_messages_->push_back(it->second);
	}

	sent_messages_.clear();
}

bool
message_cache::is_message_expired(cached_message_ptr_t msg) {
	return msg->is_expired();
}

void
message_cache::process_expired_messages(std::vector<std::string>& expired_uuids) {
	boost::mutex::scoped_lock lock(mutex_);

	// remove expired from sent
	messages_index_t::iterator it = sent_messages_.begin();
	while (it != sent_messages_.end()) {

		// get single sent message
		boost::shared_ptr<cached_message> msg = it->second;
		if (!msg) {
			throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
		}

		// remove expired messages
		if (msg->is_expired()) {
			expired_uuids.push_back(msg->uuid());
			sent_messages_.erase(it++);
		}
		else {
			++it;
		}
	}

	if (!new_messages_) {
		throw error("empty pending message queue object at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	message_queue_t::iterator it2 = new_messages_->begin();
	while (it2 != new_messages_->end()) {
		// get single pending message
		boost::shared_ptr<cached_message> msg = *it2;

		if (!msg) {
			throw error("empty cached message object at " + std::string(BOOST_CURRENT_FUNCTION));
		}

		// remove expired messages
		if (msg->is_expired()) {
			expired_uuids.push_back(msg->uuid());
		}

		++it2;
	}

	new_messages_->erase(std::remove_if(new_messages_->begin(),
										new_messages_->end(),
										&message_cache::is_message_expired),
						 new_messages_->end());
}

} // namespace lsd

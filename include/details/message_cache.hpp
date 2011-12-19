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

#ifndef _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_
#define _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

#include <string>
#include <memory>
#include <map>
#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <eblob/eblob.hpp>

#include "lsd/structs.hpp"
#include "details/context.hpp"
#include "details/cached_message.hpp"

namespace lsd {

class message_cache : private boost::noncopyable {
public:
	typedef boost::shared_ptr<cached_message> cached_message_ptr_t;
	typedef std::deque<cached_message_ptr_t> message_queue_t;
	typedef boost::shared_ptr<message_queue_t> message_queue_ptr_t;

	// map <uuid, cached message>
	typedef std::map<std::string, cached_message_ptr_t> messages_index_t;

public:
	explicit message_cache(boost::shared_ptr<lsd::context> context,
						   enum message_cache_type type);

	virtual ~message_cache();

	void enqueue(boost::shared_ptr<cached_message> message);
	void append_message_queue(message_queue_ptr_t queue);

	size_t new_messages_count();
	size_t sent_messages_count();
	cached_message& get_new_message();
	cached_message& get_sent_message(const std::string& uuid);
	message_queue_ptr_t new_messages();
	void move_new_message_to_sent();
	void move_sent_message_to_new(const std::string& uuid);
	void remove_message_from_cache(const std::string& uuid);
	void make_all_messages_new();
	void process_expired_messages(std::vector<std::string>& expired_uuids);

private:
	static bool is_message_expired(cached_message_ptr_t msg);

	boost::shared_ptr<lsd::context> context();
	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();

private:
	boost::shared_ptr<lsd::context> context_;
	enum message_cache_type type_;

	messages_index_t sent_messages_;
	message_queue_ptr_t new_messages_;

	boost::mutex mutex_;
};

} // namespace lsd

#endif // _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

#ifndef _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_
#define _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

#include <string>
#include <memory>
#include <map>
#include <deque>

#include <boost/shared_ptr.hpp>

#include <eblob/eblob.hpp>

#include "context.hpp"
#include "structs.hpp"
#include "cached_message.hpp"

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
	size_t new_messages_count() const;
	size_t sent_messages_count() const;
	cached_message_ptr_t get_new_message() const;
	cached_message_ptr_t get_sent_message(const std::string& uuid) const;
	message_queue_ptr_t new_messages() const;
	message_queue_ptr_t new_messages();
	void move_new_message_to_sent();
	void move_sent_message_to_new(const std::string& uuid);
	void remove_message_from_cache(const std::string& uuid);
	void make_all_messages_new();

private:
	boost::shared_ptr<lsd::context> context();

private:
	boost::shared_ptr<lsd::context> context_;
	enum message_cache_type type_;

	messages_index_t sent_messages_;
	message_queue_ptr_t new_messages_;
};

} // namespace lsd

#endif // _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

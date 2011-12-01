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
	typedef boost::shared_ptr<cached_message> cached_massage_ptr_t;
	typedef std::deque<cached_massage_ptr_t> message_queue_t;

	typedef std::map<std::string, cached_massage_ptr_t> messages_index_t;

public:
	explicit message_cache(boost::shared_ptr<lsd::context> context,
						   enum message_cache_type type);

	virtual ~message_cache();

	void enqueue(boost::shared_ptr<cached_message> message);

private:
	boost::shared_ptr<lsd::context> context();

private:
	boost::shared_ptr<lsd::context> context_;
	enum message_cache_type type_;

	message_queue_t sent_messages_;
	message_queue_t new_messages_;
};

} // namespace lsd

#endif // _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

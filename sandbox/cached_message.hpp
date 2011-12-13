#ifndef _LSD_CACHED_MESSAGE_HPP_INCLUDED_
#define _LSD_CACHED_MESSAGE_HPP_INCLUDED_

#include <string>
#include <sys/time.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "structs.hpp"
#include "data_container.hpp"

namespace lsd {

class cached_message {
public:
	cached_message();
	cached_message(const cached_message& message);

	cached_message(const message_path& path,
				   const message_policy& policy,
				   const void* data,
				   size_t data_size_);

	virtual ~cached_message();

	const data_container& data() const;
	size_t container_size() const;

	const message_path& path() const;
	const message_policy& policy() const;
	const std::string& uuid() const;

	bool is_sent() const;
	void set_sent(bool value);

	const timeval& sent_timestamp() const;
	void set_sent_timestamp(const timeval& val);

	void mark_as_unsent();

	std::string json();
	bool is_expired();

	cached_message& operator = (const cached_message& rhs);
	bool operator == (const cached_message& rhs) const;
	bool operator != (const cached_message& rhs) const;

	static const size_t MAX_MESSAGE_DATA_SIZE = 2147483648; // 2gb
	static const size_t UUID_SIZE = 36; // bytes

private:
	void gen_uuid();
	void init();
	
private:
	// message data
	data_container data_;
	message_path path_;
	message_policy policy_;
	std::string uuid_;

	// metadata
	bool is_sent_;
	timeval sent_timestamp_;
	size_t container_size_;
	int timeout_retries_count_;

	// synchronization
	boost::mutex mutex_;
};

} // namespace lsd

#endif // _LSD_CACHED_MESSAGE_HPP_INCLUDED_

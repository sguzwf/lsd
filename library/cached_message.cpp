#include <cstring>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <uuid/uuid.h>
#include "json/json.h"

#include "lsd/structs.hpp"
#include "details/cached_message.hpp"
#include "details/error.hpp"
#include "details/progress_timer.hpp"

namespace lsd {
cached_message::cached_message() :
	is_sent_(false)
{
	init();
}

cached_message::cached_message(const cached_message& message) {
	*this = message;
}

cached_message::cached_message(const message_path& path,
							   const message_policy& policy,
							   const void* data,
							   size_t data_size) :
	path_(path),
	policy_(policy),
	is_sent_(false),
	container_size_(0)
{
	if (data_size > MAX_MESSAGE_DATA_SIZE) {
		throw error(LSD_MESSAGE_DATA_TOO_BIG_ERROR, "can't create message, message data too big.");
	}

	data_ = data_container(data, data_size);
	init();
}

cached_message::~cached_message() {
}

void
cached_message::init() {
	sent_timestamp_.tv_sec = 0;
	sent_timestamp_.tv_usec = 0;
	gen_uuid();

	// calc data size
	container_size_ = sizeof(cached_message) + data_.size() + UUID_SIZE + path_.container_size();
}

void
cached_message::gen_uuid() {
	char buff[128];
	memset(buff, 0, sizeof(buff));

	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse(uuid, buff);

	uuid_ = buff;
}

const data_container&
cached_message::data() const {
	return data_;
}

cached_message&
cached_message::operator = (const cached_message& rhs) {
	boost::mutex::scoped_lock lock(mutex_);

	if (this == &rhs) {
		return *this;
	}

	data_			= rhs.data_;
	path_			= rhs.path_;
	policy_			= rhs.policy_;
	uuid_			= rhs.uuid_;
	is_sent_		= rhs.is_sent_;
	sent_timestamp_	= rhs.sent_timestamp_;
	container_size_	= rhs.container_size_;

	return *this;
}

bool
cached_message::operator == (const cached_message& rhs) const {
	return (uuid_ == rhs.uuid_);
}

bool
cached_message::operator != (const cached_message& rhs) const {
	return !(*this == rhs);
}

const std::string&
cached_message::uuid() const {
	return uuid_;
}

bool
cached_message::is_sent() const {
	return is_sent_;
}

const timeval&
cached_message::sent_timestamp() const {
	return sent_timestamp_;
}

const message_path&
cached_message::path() const {
	return path_;
}

const message_policy&
cached_message::policy() const {
	return policy_;
}

size_t
cached_message::container_size() const {
	return container_size_;
}

void
cached_message::set_sent(bool value) {
	boost::mutex::scoped_lock lock(mutex_);
	is_sent_ = value;
}

void
cached_message::set_sent_timestamp(const timeval& val) {
	boost::mutex::scoped_lock lock(mutex_);
	sent_timestamp_ = val;
}

void
cached_message::mark_as_unsent() {
	boost::mutex::scoped_lock lock(mutex_);
	is_sent_ = false;
	sent_timestamp_.tv_sec = 0;
	sent_timestamp_.tv_usec = 0;
}

bool
cached_message::is_expired() {
	if (policy_.deadline == 0.0f) {
		return false;
	}

	timeval tv = progress_timer::get_precise_time();

	double current_time = tv.tv_sec;
	current_time += tv.tv_usec / 1000000.0f;

	return (current_time > policy_.deadline);
}

std::string
cached_message::json() {
	std::string resulting_json;
	resulting_json += "{\n";

	if (policy_.urgent) {
		resulting_json += "\"urgent\" : true,\n";
	}
	else {
		resulting_json += "\"urgent\" : false,\n";
	}

	if (policy_.mailboxed) {
		resulting_json += "\"mailboxed\" : true,\n";
	}
	else {
		resulting_json += "\"mailboxed\" : false,\n";
	}

	if (policy_.timeout) {
		resulting_json += "\"timeout\" : 0.0,\n";
	}
	else {
		resulting_json += "\"timeout\" : 0.0,\n";
	}

	if (policy_.deadline) {
		resulting_json += "\"deadline\" : 0.0,\n";
	}
	else {
		resulting_json += "\"deadline\" : 0.0,\n";
	}

	resulting_json += "\"uuid\" : \"" + uuid_ + "\"\n}";

	return resulting_json;
}

} // namespace lsd

#ifndef _PMQ_CACHED_MESSAGE_HPP_INCLUDED_
#define _PMQ_CACHED_MESSAGE_HPP_INCLUDED_

#include <string>
#include <stdexcept>
#include <sys/time.h>

namespace pmq {

class cached_message {
public:
	cached_message();
	cached_message(const cached_message& message);
	cached_message(const std::string& service_prefix, const std::string& message);
	virtual ~cached_message();
	
	std::string json();
	static cached_message from_json_string(const std::string& cached_message_json);
	cached_message& operator=(const cached_message &rhs);
	bool operator==(const cached_message &rhs);
	bool operator!=(const cached_message &rhs);
	
public:
	const std::string& service_prefix() const;
	void set_service_prefix(const std::string& service_prefix);

	const std::string& message() const;
	void set_message(const std::string& message);
	
	const std::string& uuid() const;
	void set_uuid(const std::string& uuid);
	void gen_uuid();
	
	bool is_sent();
	void set_sent(bool value);

	const timeval& sent_timestamp() const;
	void set_sent_timestamp(const timeval& timestamp);
	
private:
	void init();
	
private:
	std::string service_prefix_;
	std::string message_; // json encoded message
	std::string message_uuid_;	
	bool is_sent_;
	timeval sent_timestamp_;
};

} // namespace pmq

#endif // _PMQ_CACHED_MESSAGE_HPP_INCLUDED_
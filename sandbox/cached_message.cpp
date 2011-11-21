#include "settings.h"

#include <cstring>

#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <uuid/uuid.h>
#include "json/json.h"
#include "structs.hpp"

#include "cached_message.hpp"

namespace lsd {
cached_message::cached_message() :
	is_sent_(false)
{
	init();
}

cached_message::cached_message(const cached_message& message) {
	*this = message;
}

cached_message::cached_message(const std::string& service_prefix, const std::string& message) :
	service_prefix_(service_prefix),
	message_(message),
	is_sent_(false)
{
	init();
}

cached_message::~cached_message() {

}

void
cached_message::init() {
	sent_timestamp_.tv_sec = 0;
	sent_timestamp_.tv_usec = 0;
	gen_uuid();
}

std::string
cached_message::json() {
	if (service_prefix_.empty()) {
		throw std::runtime_error("service prefix is empty at:" + std::string(BOOST_CURRENT_FUNCTION));
	}
	
	Json::Value root;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(message_, root);

	if (!parsingSuccessful) {
		throw std::runtime_error("message string could not be parsed at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	std::string retval;
	
	try {
		Json::Value msg(Json::objectValue);
		Json::FastWriter writer;
		
		msg["spref"] = service_prefix_;
		msg["uuid"] = message_uuid_;
		msg["protv"] = PROTOCOL_VERSION;
		msg["msg"] = root;

		retval = writer.write(msg);
	}
	catch (...) {
		throw std::runtime_error("json string could not be created at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
	
	return retval;
}

const std::string&
cached_message::service_prefix() const {
	return service_prefix_;
}

void
cached_message::set_service_prefix(const std::string& service_prefix) {
	service_prefix_ = service_prefix;
}

const std::string&
cached_message::message() const {
	return message_;
}

void
cached_message::set_message(const std::string& message) {
	message_ = message;
}
	
const std::string&
cached_message::uuid() const {
	return message_uuid_;
}

void
cached_message::set_uuid(const std::string& uuid) {
	message_uuid_ = uuid;
}

void
cached_message::gen_uuid() {
	char buff[128];
	memset(buff, 0, sizeof(buff));

	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse(uuid, buff);

	message_uuid_ = buff;
}

bool
cached_message::is_sent() {
	return is_sent_;
}

void
cached_message::set_sent(bool value) {
	is_sent_ = value;
}

const timeval&
cached_message::sent_timestamp() const {
	return sent_timestamp_;
}

void
cached_message::set_sent_timestamp(const timeval& timestamp) {
	sent_timestamp_ = timestamp;
}

cached_message
cached_message::from_json_string(const std::string& cached_message_json) {
	Json::Value root;
	Json::Reader reader;
	bool parsing_successful = reader.parse(cached_message_json, root);

	if (!parsing_successful) {
		throw std::runtime_error("message string could not be parsed at: " + std::string(BOOST_CURRENT_FUNCTION));
	}
	
	int proto_version = root.get("protv", 0).asInt();
	
	if (proto_version != PROTOCOL_VERSION) {
		//std::string err_msg = "bad protocol version: " + boost::lexical_cast<std::string>(proto_version) + " current lib proto version: ";
		//err_msg += boost::lexical_cast<std::string>(PROTOCOL_VERSION);
		//throw std::runtime_error(err_msg);
	}

	cached_message msg(root.get("spref", "").asString(), root.get("msg", "").toStyledString());
	msg.set_uuid(root.get("uuid", "").asString());
	
	return msg;
}

cached_message&
cached_message::operator=(const cached_message &rhs) {
	if (this == &rhs) {
		return *this;
	}
	
	service_prefix_ = rhs.service_prefix_;
	message_ = rhs.message_;
	message_uuid_ = rhs.message_uuid_;
	is_sent_ = rhs.is_sent_;
	sent_timestamp_ = rhs.sent_timestamp_;
	
	return *this;
}

bool
cached_message::operator==(const cached_message &rhs) {
	return (service_prefix_ == rhs.service_prefix_ && message_uuid_ == rhs.message_uuid_);
}

bool
cached_message::operator!=(const cached_message &rhs) {
	return !(*this == rhs);
}
	
} // namespace lsd
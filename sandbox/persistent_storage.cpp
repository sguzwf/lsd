#include "settings.h"

#include <algorithm>
#include <stdexcept>
#include <uuid/uuid.h>
#include <map>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/progress.hpp>

#include "json/json.h"

#include "persistent_storage.hpp"
#include "persistent_storage_callback.hpp"
#include "progress_timer.hpp"
#include "helpers.hpp"

namespace lsd {

persistent_storage::persistent_storage(boost::shared_ptr<configuration> config, boost::shared_ptr<base_logger> logger) :
	config_(config),
	logger_(logger)
{
	open_storage();
}

persistent_storage::~persistent_storage() {
	close_eblob();
}

void
persistent_storage::open_storage() {
	try {			
		logger()->log("load existing eblob at path: %s", config()->eblob_path().c_str());
		progress_timer t;
		
		// load messages from eblob to online cache
		persistent_storage_callback cb(boost::bind(&persistent_storage::eblob_iterator_callback, this, _1));
		zbr::eblob_iterator eblob_iter(config()->eblob_path(), true);
		eblob_iter.iterate(cb, 1);
		
		int msg_count = messages_in_online_cache();
		
		if (msg_count > 0) {
			logger()->log("loaded %d messages from eblob in %0.4f seconds", msg_count, t.elapsed());
		}
		else {
			logger()->log("no messages in eblob");
		}
	}
	catch (...) {
		logger()->log("no messages in eblob");
	}

	progress_timer t;		
	logger()->log(PLOG_DEBUG, "reopen eblob for writing");
	open_eblob();
	logger()->log(PLOG_DEBUG, "loaded eblob in %0.4f seconds", t.elapsed());
}

void
persistent_storage::eblob_iterator_callback(const std::string& value) {
	cached_message msg = cached_message::from_json_string(value);

	// add data to online cache
	add_message_to_online_cache(msg);
}

void
persistent_storage::open_eblob() {
	/*
	zbr::eblob_logger l(, );
	zbr::eblob_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.file = (char*)eblob_path_.c_str();
	cfg.log = l.log();
	cfg.sync = 1;
	cfg.iterate_threads = 16;

	eblob_.reset(new zbr::eblob(eblob_log_path_.c_str(), eblob_log_flags_, &cfg));
	*/

	eblob_.reset(new zbr::eblob(config()->eblob_log_path().c_str(),
								config()->eblob_log_flags(),
								config()->eblob_path()));
}

void
persistent_storage::close_eblob() {
	eblob_.reset(NULL);
}

void
persistent_storage::write_to_eblob(const std::string& key, const std::string& value) {	
	if (key.empty()) {
		return;
	}

	eblob_->write_hashed(key, value, BLOB_DISK_CTL_NOCSUM);
}

std::string
persistent_storage::read_from_eblob(const std::string& key) {
	if (key.empty()) {
		return "";
	}
	
	return eblob_->read_hashed(key, 0, 0);
}

void
persistent_storage::erase_from_eblob(const std::string& key) {	
	eblob_->remove_hashed(key);
}

size_t
persistent_storage::eblob_size() {
	return static_cast<size_t>(eblob_->elements());
}

void
persistent_storage::add_message(const std::string& msg, const std::string& service_prefix) {
	cached_message cmesg(service_prefix, msg);
	add_message(cmesg);
}

void
persistent_storage::add_message(cached_message& msg) {
	add_message_to_online_cache(msg);

	// add message to offline cache
	write_to_eblob(msg.uuid(), msg.json());
}

void
persistent_storage::add_message_to_online_cache(const cached_message& msg) {	
	// add message to online cache
	message_map_iter it = cache_.find(msg.service_prefix());
	
	if (it == cache_.end()) {
		std::deque<cached_message> queue;
		queue.push_back(msg);
		cache_[msg.service_prefix()] = queue;
	}
	else {
		it->second.push_back(msg);
	}
}

void
persistent_storage::remove_message(const cached_message& msg) {
	remove_message(msg.service_prefix(), msg.uuid());
}

void
persistent_storage::remove_message(const std::string& service_prefix, const std::string& uuid) {
	message_map_iter iter = cache_.find(service_prefix);
	
	// remove message from online cache.
	// check for group queue existence and whether queue is not empty
	if (iter != cache_.end() && !iter->second.empty()) {
		for (std::deque<cached_message>::iterator it = iter->second.begin(); it != iter->second.end();) {
			if (it->service_prefix() == service_prefix && it->uuid() == uuid) {
				iter->second.erase(it);
				break;
			}
			else {
				++it;
			}
		}
	}

	// remove message from offline cache.
	erase_from_eblob(uuid);
}

bool
persistent_storage::pending_message(cached_message& msg, const std::string& service_prefix) {
	message_map_iter it = cache_.find(service_prefix);
	
	// check for group queue existence
	if (it == cache_.end()) {
		return false;
	}

	// check whether queue is empty
	if (it->second.empty()) {
		return false;
	}

	// get frontmost non-sent message
	for (message_queue_iter qit = it->second.begin(); qit != it->second.end(); ++qit) {
		if (!qit->is_sent()) {
			msg = *qit;
			return true;
		}
	}
	
	return false;
}

void
persistent_storage::update_message(cached_message& msg) {
	message_map_iter it = cache_.find(msg.service_prefix());
	
	// check for group queue existence
	if (it == cache_.end()) {
		return;
	}

	// check whether queue is empty
	if (it->second.empty()) {
		return;
	}

	// get frontmost non-sent message
	for (message_queue_iter qit = it->second.begin(); qit != it->second.end(); ++qit) {
		if (*qit == msg) {
			qit->set_message(msg.message());
			qit->set_sent(msg.is_sent());
			qit->set_sent_timestamp(msg.sent_timestamp());
			return;
		}
	}
}

bool
persistent_storage::pending_message_queue(message_queue& queue, const std::string& service_prefix) {
	message_map_iter it = cache_.find(service_prefix);

	// check for group queue existence
	if (it == cache_.end()) {
		return false;
	}

	// check whether queue is empty
	if (it->second.empty()) {
		return false;
	}
	
	queue.clear();
	queue.insert(queue.begin(), it->second.begin(), it->second.end());
	
	return true;
}

void
persistent_storage::pending_services(std::vector<std::string>& services) {
	services.clear();
	services.reserve(cache_.size());
	
	for (message_map_iter it = cache_.begin(); it != cache_.end(); ++it) {
		services.push_back(it->first);
	}
}

size_t
persistent_storage::messages_in_online_cache() {
	size_t count = 0;
	
	for (message_map_iter it = cache_.begin(); it != cache_.end(); ++it) {
		count += it->second.size();
	}
	
	return count;
}

size_t
persistent_storage::messages_count_all() {
	size_t retval = 0;
	
	for (message_map_iter it = cache_.begin(); it != cache_.end(); ++it) {
		retval += it->second.size();
	}
	
	return retval;
}

size_t
persistent_storage::messages_count(const std::string& service_prefix) {
	message_map_iter it = cache_.find(service_prefix);

	// check for group queue existence
	if (it == cache_.end()) {
		return 0;
	}

	// check whether queue is empty
	if (it->second.empty()) {
		return 0;
	}
	
	return it->second.size();
}

void
persistent_storage::mark_messages_unsent(const std::string& service_prefix) {
	message_map_iter it = cache_.find(service_prefix);

	// check for group queue existence
	if (it == cache_.end()) {
		return;
	}

	// check whether queue is empty
	if (it->second.empty()) {
		return;
	}
	
	for (message_queue_iter qit = it->second.begin(); qit != it->second.end(); ++qit) {
		qit->set_sent(false);
	}
}

boost::shared_ptr<base_logger>
persistent_storage::logger() {
	return logger_;
}

boost::shared_ptr<configuration>
persistent_storage::config() {
	return config_;
}

} // namespace lsd

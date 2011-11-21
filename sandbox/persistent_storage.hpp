#ifndef _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_
#define _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_

#include <string>
#include <memory>
#include <map>
#include <deque>

#include <boost/shared_ptr.hpp>

#include <eblob/eblob.hpp>

#include "smart_logger.hpp"
#include "structs.hpp"
#include "cached_message.hpp"
#include "configuration.hpp"

namespace lsd {

class persistent_storage : private boost::noncopyable {
public:
	typedef std::deque<cached_message> message_queue;
	typedef message_queue::iterator message_queue_iter;
	typedef message_queue::const_iterator message_queue_citer;
	typedef std::map<std::string, message_queue> message_map;
	typedef message_map::iterator message_map_iter;
	typedef message_map::const_iterator message_map_citer;
	
public:
	explicit persistent_storage(boost::shared_ptr<configuration> context, boost::shared_ptr<base_logger> logger);
	virtual ~persistent_storage();
	
	void add_message(cached_message& msg);
	void add_message(const std::string& msg, const std::string& service_prefix);
	bool pending_message(cached_message& msg, const std::string& service_prefix);
	void update_message(cached_message& msg);
	void remove_message(const cached_message& msg);
	void remove_message(const std::string& service_prefix, const std::string& uuid);

	size_t messages_count_all();
	size_t messages_count(const std::string& service_prefix);
	
	void mark_messages_unsent(const std::string& service_prefix);
	bool pending_message_queue(message_queue& queue, const std::string& service_prefix);
	void pending_services(std::vector<std::string>& services);
	
private:
	void open_storage();
	void open_eblob();
	void close_eblob();
	
	void write_to_eblob(const std::string& key, const std::string& value);
	std::string read_from_eblob(const std::string& key);
	void erase_from_eblob(const std::string& key);
	
	size_t eblob_size();

	void eblob_iterator_callback(const std::string& value);
	void add_message_to_online_cache(const cached_message& msg);
	
	size_t messages_in_online_cache();
	
	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	
private:
	int sync_interval_;
	std::auto_ptr<zbr::eblob> eblob_; // offline cache
	message_map cache_; // online cache
	
	boost::shared_ptr<configuration> config_;
	boost::shared_ptr<base_logger> logger_;
};

} // namespace lsd

#endif // _LSD_PERSISTENT_STORAGE_HPP_INCLUDED_
#ifndef _LSD_SERVICE_HPP_INCLUDED_
#define _LSD_SERVICE_HPP_INCLUDED_

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <deque>

#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "service_info.hpp"
#include "host_info.hpp"
#include "handle_info.hpp"
#include "handle.hpp"
#include "smart_logger.hpp"
#include "structs.hpp"
#include "context.hpp"
#include "cached_message.hpp"
#include "error.hpp"

namespace lsd {

// predeclaration
template <typename LSD_T> class service;
typedef service<LT> service_t;
template<typename T> std::ostream& operator << (std::ostream& out, const service<T>& s);

template <typename LSD_T>
class service : public boost::noncopyable {
public:
	typedef std::vector<host_info<LSD_T> > hosts_info_list_t;
	typedef std::vector<handle_info<LSD_T> > handles_info_list_t;

	typedef boost::shared_ptr<handle<LSD_T> > handle_ptr_t;
	typedef std::map<typename LSD_T::ip_addr, std::string> hosts_map_t;
	typedef std::map<std::string, handle_ptr_t> handles_map_t;

	typedef boost::shared_ptr<cached_message> cached_message_prt_t;
	typedef std::deque<cached_message_prt_t> cached_messages_deque_t;
	typedef boost::shared_ptr<cached_messages_deque_t> messages_deque_ptr_t;
	typedef std::map<std::string, messages_deque_ptr_t> unhandled_messages_map_t;

public:
	service(const service_info<LSD_T>& info, boost::shared_ptr<lsd::context> context);

	void refresh_hosts_and_handles(const hosts_info_list_t& hosts,
								   const std::vector<handle_info<LSD_T> >& handles);

	void send_message(cached_message_prt_t message);
	size_t queue_storage_size();

public:
	template<typename T> friend std::ostream& operator << (std::ostream& out, const service<T>& s);

private:
	void refresh_hosts(const hosts_info_list_t& hosts,
					   hosts_info_list_t& oustanding_hosts,
					   hosts_info_list_t& new_hosts);

	void refresh_handles(const handles_info_list_t& handles,
						 handles_info_list_t& oustanding_handles,
						 handles_info_list_t& new_handles);

	void remove_outstanding_handles(const handles_info_list_t& handles);
	void create_new_handles(const handles_info_list_t& handles, const hosts_info_list_t& hosts);

	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<lsd::context> context();

private:
	service_info<LSD_T> info_;
	hosts_map_t hosts_;		// hosts (ip, hostname)
	handles_map_t handles_;	// collected from hosts data (handle name, port)
	boost::shared_ptr<lsd::context> context_;
	size_t queue_storage_size_;
	unhandled_messages_map_t unhandled_messages_;
};

template <typename LSD_T>
service<LSD_T>::service(const service_info<LSD_T>& info, boost::shared_ptr<lsd::context> context) :
	info_(info),
	context_(context),
	queue_storage_size_(0)
{
}

template <typename LSD_T> boost::shared_ptr<lsd::context>
service<LSD_T>::context() {
	if (!context_.get()) {
		throw error("lsd context object is empty at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return context_;
}

template <typename LSD_T> boost::shared_ptr<base_logger>
service<LSD_T>::logger() {
	return context()->logger();
}

template <typename LSD_T> boost::shared_ptr<configuration>
service<LSD_T>::config() {
	boost::shared_ptr<configuration> conf = context()->config();
	if (!conf.get()) {
		throw error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return conf;
}

template <typename LSD_T> void
service<LSD_T>::refresh_hosts_and_handles(const hosts_info_list_t& hosts,
		const handles_info_list_t& handles)
{
	logger()->log(PLOG_DEBUG, "service %s refreshed with:", info_.name_.c_str());

	for (size_t i = 0; i < hosts.size(); ++i) {
		std::stringstream tmp;
		tmp << "host - " << hosts[i];
		logger()->log(PLOG_DEBUG, tmp.str());
	}

	for (size_t i = 0; i < handles.size(); ++i) {
		std::stringstream tmp;
		tmp << "handle - " << handles[i];
		logger()->log(PLOG_DEBUG, tmp.str());
	}

	// refresh hosts
	hosts_info_list_t outstanding_hosts;
	hosts_info_list_t new_hosts;
	refresh_hosts(hosts, outstanding_hosts, new_hosts);


	// refresh handles
	handles_info_list_t outstanding_handles;
	handles_info_list_t new_handles;
	refresh_handles(handles, outstanding_handles, new_handles);

	// remove oustanding handles
	remove_outstanding_handles(outstanding_handles);

	// make list of hosts
	hosts_info_list_t hosts_v;
	for (typename hosts_map_t::iterator it = hosts_.begin(); it != hosts_.end(); ++it) {
		hosts_v.push_back(host_info<LSD_T>(it->first, it->second));
	}

	// reconnect existing handles if we have outstanding hosts
	if (!outstanding_hosts.empty()) {
		typename handles_map_t::iterator it = handles_.begin();
		for (;it != handles_.end(); ++it) {
			it->second->reconnect(hosts_v);
		}
	}
	else {
		// add connections to new hosts
		if (!new_hosts.empty()) {
			typename handles_map_t::iterator it = handles_.begin();
			for (;it != handles_.end(); ++it) {
				it->second->connect(new_hosts);
			}
		}
	}

	// create new handles if any
	create_new_handles(new_handles, hosts_v);
}

template <typename LSD_T> void
service<LSD_T>::refresh_hosts(const hosts_info_list_t& hosts,
							  hosts_info_list_t& oustanding_hosts,
							  hosts_info_list_t& new_hosts)
{
	// check for outstanding hosts
	for (typename hosts_map_t::iterator it = hosts_.begin(); it != hosts_.end(); ++it) {
		bool found = false;
		for (size_t i = 0; i < hosts.size(); ++i) {
			if (hosts[i].ip_ == it->first) {
				found = true;
			}
		}

		if (!found) {
			oustanding_hosts.push_back(host_info<LSD_T>(it->first, it->second));
		}
	}

	// check for new hosts
	for (size_t i = 0; i < hosts.size(); ++i) {
		typename hosts_map_t::iterator it = hosts_.find(hosts[i].ip_);
		if (it == hosts_.end()) {
			new_hosts.push_back(hosts[i]);
		}
	}

	hosts_.clear();

	// replace hosts list with new hosts
	for (size_t i = 0; i < hosts.size(); ++i) {
		hosts_[hosts[i].ip_] = hosts[i].hostname_;
	}
}

template <typename LSD_T> void
service<LSD_T>::refresh_handles(const handles_info_list_t& handles,
					 	 	    handles_info_list_t& oustanding_handles,
					 	 	    handles_info_list_t& new_handles)
{
	// check for outstanding hosts
	for (typename handles_map_t::iterator it = handles_.begin(); it != handles_.end(); ++it) {
		bool found = false;
		for (size_t i = 0; i < handles.size(); ++i) {
			if (it->second->info() == handles[i]) {
				found = true;
			}
		}

		if (!found) {
			oustanding_handles.push_back(it->second->info());
		}
	}

	// check for new handles
	for (size_t i = 0; i < handles.size(); ++i) {
		typename handles_map_t::iterator it = handles_.find(handles[i].name_);
		if (it == handles_.end()) {
			new_handles.push_back(handles[i]);
		}
	}
}

template <typename LSD_T> void
service<LSD_T>::remove_outstanding_handles(const handles_info_list_t& handles) {
	// no handles to destroy
	if (handles.empty()) {
		return;
	}

	// destroy handles
	for (size_t i = 0; i < handles.size(); ++i) {
		typename handles_map_t::iterator it = handles_.find(handles[i].name_);

		if (it != handles_.end()) {
			handle_ptr_t handle = it->second;

			// check handle
			if (!handle.get()) {
				std::string error_str = "service handle object is empty. service: " + info_.name_;
				error_str += ", handle: " + handles[i].name_;
				error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_str);
			}

			// immediately terminate all handle activity
			handle->terminate_with_timeout(0.0f);
			boost::shared_ptr<message_cache> msg_cache = handle->messages_cache();

			// check handle message cache
			if (!msg_cache.get()) {
				std::string error_str = "handle message cache object is empty. service: " + info_.name_;
				error_str += ", handle: " + handles[i].name_;
				error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_str);
			}

			// consolidate all handle messages
			msg_cache->make_all_messages_new();

			// find corresponding unhandled msgs queue
			unhandled_messages_map_t::iterator it = unhandled_messages_.find(handle->info().name_);

			// should not find a queue with messages!
			if (it != unhandled_messages_.end()) {
				messages_deque_ptr_t msg_queue = it->second;

				if (msg_queue.get() && !msg_queue->empty()) {
					std::string error_str = "found unhandled non-empty message queue with existing handle!";
					error_str += " service: " + info_.name_; ", handle: " + handles[i].name_;
					error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
					throw error(error_str);
				}

				// remove empty queue if any
				unhandled_messages_.erase(it);
			}

			// move handle messages to unhandled messages map in service
			messages_deque_ptr_t handle_msg_queue = msg_cache->new_messages();

			// validate handle queue
			if (!handle_msg_queue.get()) {
				std::string error_str = "found empty handle message queue when handle exists!";
				error_str += " service: " + info_.name_; ", handle: " + handles[i].name_;
				error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_str);
			}

			// in case there are messages, store them
			if (!handle_msg_queue->empty()) {
				unhandled_messages_[handle->info().name_] = handle_msg_queue;
			}
		}

		handles_.erase(it);
	}
}

template <typename LSD_T> void
service<LSD_T>::create_new_handles(const handles_info_list_t& handles, const hosts_info_list_t& hosts) {
	// no handles to create
	if (handles.empty()) {
		return;
	}

	// create handles
	for (size_t i = 0; i < handles.size(); ++i) {
		handle_ptr_t handle_ptr;
		handle_ptr.reset(new handle<LSD_T>(handles[i], context_, hosts));

		// find corresponding unhandled msgs queue
		unhandled_messages_map_t::iterator it = unhandled_messages_.find(handles[i].name_);

		// validate queue
		if (it != unhandled_messages_.end()) {
			messages_deque_ptr_t msg_queue = it->second;

			// add existing message queue to handle
			if (msg_queue.get() && !msg_queue->empty()) {

				// validate handle's message cache object
				if (handle_ptr->messages_cache().get()) {
					handle_ptr->messages_cache()->append_message_queue(msg_queue);
				}
				else {
					std::string error_str = "found empty handle message queue when handle exists!";
					error_str += " service: " + info_.name_; ", handle: " + handles[i].name_;
					error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
					throw error(error_str);
				}
			}
		}

		// remove message queue from unhandled messages map
		unhandled_messages_.erase(it);

		// add handle to storage and connect it
		handles_[handles[i].name_] = handle_ptr;
		handles_[handles[i].name_]->connect(hosts);
	}
}

template <typename LSD_T> void
service<LSD_T>::send_message(cached_message_prt_t message) {
	if (!message.get()) {
		std::string error_str = "message object is empty. service: " + info_.name_;
		error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	const std::string& handle_name = message->path().handle_name;

	// find existing handle to enqueue message
	typename handles_map_t::iterator it = handles_.find(handle_name);
	if (it != handles_.end()) {
		handle_ptr_t handle_ptr = it->second;

		// make sure we have valid handle
		if (handle_ptr.get()) {
			handle_ptr->enqueue_message(message);
			queue_storage_size_ += message->data_size();
		}
		else {
			std::string error_str = "handle object " + handle_name;
			error_str += " for service: " + info_.name_ + " is empty.";
			error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
			throw error(error_str);
		}
	}
	else {
		// if no handle, store locally
		unhandled_messages_map_t::iterator it = unhandled_messages_.find(handle_name);

		// check for existing messages queue for handle
		if (it == unhandled_messages_.end()) {
			messages_deque_ptr_t queue_ptr;
			queue_ptr.reset(new cached_messages_deque_t);
			queue_ptr->push_back(message);
			unhandled_messages_[handle_name] = queue_ptr;
		}
		else {
			messages_deque_ptr_t queue_ptr = it->second;

			// validate msg queue
			if (!queue_ptr.get()) {
				std::string error_str = "found empty message queue in unhandled messages map!";
				error_str += " service: " + info_.name_; ", handle: " + handle_name;
				error_str += ". at " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_str);
			}

			queue_ptr->push_back(message);
		}

		queue_storage_size_ += message->data_size();
	}
}

template <typename LSD_T> size_t
service<LSD_T>::queue_storage_size() {
	return queue_storage_size_;
}

template<typename T>
std::ostream& operator << (std::ostream& out, const service<T>& s) {
	out << "----- service info: -----\n";
	out << s.info_;

	return out;
}

} // namespace lsd

#endif // _LSD_SERVICE_HPP_INCLUDED_

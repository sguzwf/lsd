#ifndef _LSD_HANDLE_HPP_INCLUDED_
#define _LSD_HANDLE_HPP_INCLUDED_

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "handle_info.hpp"
#include "host_info.hpp"
#include "structs.hpp"
#include "context.hpp"
#include "cached_message.hpp"
#include "message_cache.hpp"
#include "error.hpp"

namespace lsd {

// predeclaration
template <typename LSD_T> class handle;
typedef handle<LT> handle_t;

template <typename LSD_T>
class handle : public boost::noncopyable {
public:
	typedef std::vector<host_info<LSD_T> > hosts_info_list_t;

public:
	handle(const handle_info<LSD_T>& info,
		   boost::shared_ptr<lsd::context> context,
		   const hosts_info_list_t& hosts);

	void kill();

	const handle_info<LSD_T>& info() const;

	void reconnect(const hosts_info_list_t& hosts);
	void connect(const hosts_info_list_t& hosts);
	void enqueue_message(boost::shared_ptr<cached_message> message);

private:
	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<lsd::context> context();

private:
	handle_info<LSD_T> info_;
	boost::shared_ptr<lsd::context> context_;
	hosts_info_list_t hosts_;
	boost::shared_ptr<message_cache> message_cache_;
};

template <typename LSD_T>
handle<LSD_T>::handle(const handle_info<LSD_T>& info,
					  boost::shared_ptr<lsd::context> context,
					  const hosts_info_list_t& hosts) :
	info_(info),
	context_(context),
	hosts_(hosts)
{
	message_cache_.reset(new message_cache(context, config()->message_cache_type()));
}

template <typename LSD_T> const handle_info<LSD_T>&
handle<LSD_T>::info() const {
	return info_;
}

template <typename LSD_T> void
handle<LSD_T>::kill() {

}

template <typename LSD_T> void
handle<LSD_T>::reconnect(const hosts_info_list_t& hosts) {
	size_t i = hosts.size();
	i++;
}

template <typename LSD_T> void
handle<LSD_T>::connect(const hosts_info_list_t& hosts) {
	size_t i = hosts.size();
	i++;
}

template <typename LSD_T> void
handle<LSD_T>::enqueue_message(boost::shared_ptr<cached_message> message) {
	message_cache_->enqueue(message);
}

template <typename LSD_T> boost::shared_ptr<lsd::context>
handle<LSD_T>::context() {
	if (!context_.get()) {
		throw error("lsd context object is empty at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return context_;
}

template <typename LSD_T> boost::shared_ptr<base_logger>
handle<LSD_T>::logger() {
	return context()->logger();
}

template <typename LSD_T> boost::shared_ptr<configuration>
handle<LSD_T>::config() {
	boost::shared_ptr<configuration> conf = context()->config();
	if (!conf.get()) {
		throw error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return conf;
}

} // namespace lsd

#endif // _LSD_HANDLE_HPP_INCLUDED_

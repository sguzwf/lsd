#ifndef _LSD_REFRESHER_HPP_INCLUDED_
#define _LSD_REFRESHER_HPP_INCLUDED_

#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace lsd {

class refresher : private boost::noncopyable {

public:
	refresher(boost::function<void()> f, boost::uint32_t timeout_seconds); // timeout in secs
	virtual ~refresher();

private:
	void refreshing_thread();

private:
	boost::function<void()> f_;
	boost::uint32_t timeout_;
	volatile bool stopping_;
	boost::condition condition_;
	boost::mutex mutex_;
	boost::thread refreshing_thread_;

};

} // namespace lsd

#endif // _LSD_REFRESHER_HPP_INCLUDED_

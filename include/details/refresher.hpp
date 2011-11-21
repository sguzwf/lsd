#ifndef _PMQ_REFRESHER_HPP_INCLUDED_
#define _PMQ_REFRESHER_HPP_INCLUDED_

#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace pmq {

class refresher : private boost::noncopyable {

public:
	refresher(boost::function<void()> f, boost::uint32_t timeout); // timeout in secs
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

} // namespace pmq

#endif // _PMQ_REFRESHER_HPP_INCLUDED_

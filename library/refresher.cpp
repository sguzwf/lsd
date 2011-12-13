#include "details/refresher.hpp"

#include <iostream>
#include <boost/bind.hpp>

namespace lsd {

refresher::refresher(boost::function<void()> f, boost::uint32_t timeout_seconds) :
	f_(f),
	timeout_(timeout_seconds),
	stopping_(false),
	refreshing_thread_(boost::bind(&refresher::refreshing_thread, this)) {
}

refresher::~refresher() {
	stopping_ = true;
	condition_.notify_one();
	refreshing_thread_.join();
}

void
refresher::refreshing_thread() {
	if (!stopping_ && f_) {
		f_();
	}

	while (!stopping_) {
		boost::mutex::scoped_lock lock(mutex_);
		boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC);
		
		t.sec += timeout_;
		condition_.timed_wait(lock, t);
		
		if (!stopping_ && f_) {
			f_();
		}
	}
}

} // namespace lsd

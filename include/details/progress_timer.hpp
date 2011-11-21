#ifndef _PMQ_PROGRESS_TIMER_HPP_INCLUDED_
#define _PMQ_PROGRESS_TIMER_HPP_INCLUDED_

#include <sys/time.h>

namespace pmq {

class progress_timer {

public:
	progress_timer();
	virtual ~progress_timer();

	void reset();
	
	unsigned long long elapsed_microseconds();
	unsigned long long elapsed_milliseconds();
	double elapsed();

	static timeval get_precise_time();
	static unsigned long long timeval_diff(timeval* end_time, timeval* start_time);
	static unsigned long long timeval_diff(timeval* difference, timeval* end_time, timeval* start_time);

	static unsigned long long elapsed_microseconds_from_time(timeval* time);
	static unsigned long long elapsed_milliseconds_from_time(timeval* time);
	static double elapsed_from_time(timeval* time);

private:
	timeval begin_;
};

} // namespace pmq

#endif // _PMQ_PROGRESS_TIMER_HPP_INCLUDED_

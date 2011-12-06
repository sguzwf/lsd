#ifndef _LSD_PROGRESS_TIMER_HPP_INCLUDED_
#define _LSD_PROGRESS_TIMER_HPP_INCLUDED_

#include <sys/time.h>

namespace lsd {

class progress_timer {

public:
	progress_timer();
	virtual ~progress_timer();

	void reset();
	
	unsigned long long elapsed_microseconds();
	unsigned long long elapsed_milliseconds();
	double elapsed();

	static timeval get_precise_time();
	static unsigned long long timeval_diff(const timeval* end_time,
										   const timeval* start_time);

	static unsigned long long timeval_diff(timeval* difference,
										   const timeval* end_time,
										   const timeval* start_time);

	static unsigned long long elapsed_microseconds_from_time(const timeval* time);
	static unsigned long long elapsed_milliseconds_from_time(const timeval* time);
	static double elapsed_from_time(const timeval* time);

private:
	timeval begin_;
};

} // namespace lsd

#endif // _LSD_PROGRESS_TIMER_HPP_INCLUDED_

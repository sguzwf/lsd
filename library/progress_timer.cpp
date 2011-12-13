#include <cstddef>

#include "details/progress_timer.hpp"

namespace lsd {

progress_timer::progress_timer() {
	begin_ = get_precise_time();
}

progress_timer::~progress_timer() {

}

unsigned long long
	progress_timer::elapsed_microseconds() {
	timeval end = get_precise_time();
	return timeval_diff(&end, &begin_);
}

unsigned long long
progress_timer::elapsed_milliseconds() {
	timeval end = get_precise_time();
	return timeval_diff(&end, &begin_) / 1000;
}

double
progress_timer::elapsed() {
	timeval end = get_precise_time();
	return timeval_diff(&end, &begin_) / 1000000.0;
}

timeval
progress_timer::get_precise_time() {
	timeval tv;
	gettimeofday(&tv, NULL);
	
	return tv;
}

void
progress_timer::reset() {
	begin_ = get_precise_time();
}

unsigned long long
progress_timer::timeval_diff(const timeval* end_time,
							 const timeval* start_time)
{
	return timeval_diff(NULL, end_time, start_time);
}

unsigned long long
progress_timer::timeval_diff(timeval* difference,
							 const timeval* end_time,
							 const timeval* start_time) {
	timeval temp_diff;

	if (difference == NULL) {
		difference = &temp_diff;
	}

	difference->tv_sec = end_time->tv_sec - start_time->tv_sec;
	difference->tv_usec = end_time->tv_usec - start_time->tv_usec;
 
	/* Using while instead of if below makes the code slightly more robust. */
	while (difference->tv_usec < 0)
	{
		difference->tv_usec += 1000000;
		difference->tv_sec -= 1;
	}

	return 1000000LL * difference->tv_sec + difference->tv_usec;
}

unsigned long long
progress_timer::elapsed_microseconds_from_time(const timeval* time) {
	timeval end_time = get_precise_time();
	return timeval_diff(&end_time, time);
}

unsigned long long
progress_timer::elapsed_milliseconds_from_time(const timeval* time) {
	timeval end_time = get_precise_time();
	return timeval_diff(&end_time, time) / 1000;
}

double
progress_timer::elapsed_from_time(const timeval* time) {
	timeval end_time = get_precise_time();
	return timeval_diff(&end_time, time) / 1000000.0;
}
	
} // namespace lsd

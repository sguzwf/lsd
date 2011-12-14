#include <cstddef>

#include "details/progress_timer.hpp"

namespace lsd {

progress_timer::progress_timer() {
	begin_.init_from_current_time();
}

progress_timer::~progress_timer() {

}

void
progress_timer::reset() {
	begin_.init_from_current_time();
}

time_value
progress_timer::elapsed() {
	time_value curr_time = time_value::get_current_time();

	if (begin_ > curr_time) {
		return time_value();
	}

	time_value retval(curr_time.distance(begin_));
	return retval;
}
	
} // namespace lsd

#ifndef _LSD_PROGRESS_TIMER_HPP_INCLUDED_
#define _LSD_PROGRESS_TIMER_HPP_INCLUDED_

#include "details/time_value.hpp"

namespace lsd {

class progress_timer {

public:
	progress_timer();
	virtual ~progress_timer();

	void reset();
	time_value elapsed();

private:
	time_value begin_;
};

} // namespace lsd

#endif // _LSD_PROGRESS_TIMER_HPP_INCLUDED_

#ifndef _LSD_TIME_VALUE_HPP_INCLUDED_
#define _LSD_TIME_VALUE_HPP_INCLUDED_

#include <string>
#include <sys/time.h>

namespace lsd {

class time_value {
public:
	time_value();
	time_value(const time_value& tv);
	virtual ~time_value();

	static time_value get_current_time();

	timeval value;
};

} // namespace lsd

#endif // _LSD_TIME_VALUE_HPP_INCLUDED_

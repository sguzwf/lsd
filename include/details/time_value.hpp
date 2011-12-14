#ifndef _LSD_TIME_VALUE_HPP_INCLUDED_
#define _LSD_TIME_VALUE_HPP_INCLUDED_

#include <string>
#include <sys/time.h>

namespace lsd {

class time_value {
public:
	time_value();
	time_value(const time_value& tv);
	time_value(const timeval& tv);
	time_value(double tv);
	virtual ~time_value();

	double as_double() const;
	timeval as_timeval() const;

	long days() const;
	long hours() const;
	long minutes() const;
	long seconds() const;

	time_value& operator = (const time_value& rhs);

	bool operator == (const time_value& rhs) const;
	bool operator != (const time_value& rhs) const;
	bool operator > (const time_value& rhs) const;
	bool operator >= (const time_value& rhs) const;
	bool operator < (const time_value& rhs) const;
	bool operator <= (const time_value& rhs) const;

	double distance(const time_value& rhs);
	void init_from_current_time();

	time_value& operator + (double interval);
	time_value& operator - (double interval);

	static time_value get_current_time();

private:
	timeval value_;
};

} // namespace lsd

#endif // _LSD_TIME_VALUE_HPP_INCLUDED_

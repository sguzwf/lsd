#include <cstddef>

#include "details/time_value.hpp"

namespace lsd {

time_value::time_value() {
	value_.tv_sec = 0;
	value_.tv_usec = 0;
}

time_value::time_value(const time_value& tv) {
	*this = tv;
}

time_value::time_value(const timeval& tv) {
	value_ = tv;
}

time_value::time_value(double tv) {
	value_.tv_sec = static_cast<long>(tv);
	value_.tv_usec = static_cast<int>((tv - value_.tv_sec) * 1000000);
}

time_value::~time_value() {

}

double
time_value::as_double() const {
	double tv_as_double = value_.tv_sec;
	tv_as_double += static_cast<double>(value_.tv_usec / 1000000.0);

	return tv_as_double;
}

timeval
time_value::as_timeval() const {
	return value_;
}

long
time_value::days() const {
	return hours() / 24;
}

long time_value::hours() const {
	return minutes() / 60;
}

long time_value::minutes() const {
	return seconds() / 60;
}

long
time_value::seconds() const {
	return value_.tv_sec;
}

long
time_value::milliseconds() const {
	return value_.tv_usec / 1000;
}

long time_value::microseconds() const {
	return value_.tv_usec;
}

void
time_value::drop_microseconds() {
	value_.tv_usec = 0;
}

bool
time_value::operator == (const time_value& rhs) const {
	return (value_.tv_sec == rhs.value_.tv_sec &&
			value_.tv_usec == rhs.value_.tv_usec);
}

bool
time_value::operator != (const time_value& rhs) const {
	return !(*this == rhs);
}

time_value&
time_value::operator = (const time_value& rhs) {
	if (this == &rhs) {
		return *this;
	}

	value_ = rhs.value_;

	return *this;
}

bool
time_value::operator > (const time_value& rhs) const {
	return (as_double() > rhs.as_double());
}

bool
time_value::operator >= (const time_value& rhs) const {
	return (as_double() >= rhs.as_double());
}

bool
time_value::operator < (const time_value& rhs) const {
	return (as_double() < rhs.as_double());
}

bool
time_value::operator <= (const time_value& rhs) const {
	return (as_double() <= rhs.as_double());
}

time_value
time_value::get_current_time() {
	timeval tv;
	gettimeofday(&tv, NULL);

	return time_value(tv);
}

void
time_value::init_from_current_time() {
	*this = get_current_time();
}

double
time_value::distance(const time_value& rhs) {
	if (this == &rhs) {
		return 0.0;
	}

	timeval temp_diff;
	temp_diff.tv_sec = rhs.value_.tv_sec - value_.tv_sec;
	temp_diff.tv_usec = rhs.value_.tv_usec - value_.tv_usec;

	// Using while instead of if below makes the code slightly more robust
	while (temp_diff.tv_usec < 0) {
		temp_diff.tv_usec += 1000000;
		temp_diff.tv_sec -= 1;
	}

	long long distance = 1000000LL * temp_diff.tv_sec + temp_diff.tv_usec;
	return distance / 1000000.0;
}

time_value
time_value::operator + (double interval) {
	return time_value(as_double() + interval);
}

time_value
time_value::operator - (double interval) {
	if (as_double() >= interval) {
		return time_value(as_double() - interval);
	}
	else {
		return time_value();
	}
}

time_value&
time_value::operator += (double interval) {
	*this = time_value(as_double() + interval);
	return *this;
}

time_value&
time_value::operator -= (double interval) {
	if (as_double() >= interval) {
		*this = time_value(as_double() - interval);
	}
	else {
		*this = time_value();
	}

	return *this;
}

} // namespace lsd

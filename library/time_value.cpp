#include "settings.h"

#include <cstddef>

#include "time_value.hpp"

namespace lsd {

time_value::time_value() {

}

time_value::time_value(const time_value& tv) {
	*this = tv;
}

virtual ~time_value() {

}

time_value
time_value::get_current_time() {
	timeval tv;
	gettimeofday(&tv, NULL);

	return tv;
}

} // namespace lsd

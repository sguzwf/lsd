#include <cstddef>

#include "details/time_value.hpp"

namespace lsd {

time_value::time_value() {

}

time_value::time_value(const time_value& tv) {
	*this = tv;
}

time_value::~time_value() {

}

time_value
time_value::get_current_time() {
	time_value tv;
	//gettimeofday(&tv, NULL);

	return tv;
}

} // namespace lsd

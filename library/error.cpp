#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "details/error.hpp"

namespace lsd {

int const error::MESSAGE_SIZE;

error::error(const std::string& format, ...) :
	std::exception(),
	type_(LSD_UNKNOWN_ERROR)
{
	va_list args;
	va_start(args, format);
	vsnprintf(message_, MESSAGE_SIZE, format.c_str(), args);
	va_end(args);
}

error::error(enum error_type type, const std::string& format, ...) :
	std::exception(),
	type_(type)
{
	va_list args;
	va_start(args, format);
	vsnprintf(message_, MESSAGE_SIZE, format.c_str(), args);
	va_end(args);
}

error::~error() throw () {
}

error::error(const error& other) : std::exception(other) {
	memcpy(message_, other.message_, MESSAGE_SIZE);
}

error&
error::operator = (const error& other) {
	memcpy(message_, other.message_, MESSAGE_SIZE);
	return *this;
}

char const*
error::what() const throw () {
	return message_;
}

} // namespace lsd

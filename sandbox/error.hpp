#ifndef _LSD_ERROR_HPP_INCLUDED_
#define _LSD_ERROR_HPP_INCLUDED_

#include <exception>
#include <string>

namespace lsd {

enum error_type {
	LSD_UNKNOWN_ERROR = 1,
	LSD_MESSAGE_DATA_TOO_BIG_ERROR,
	LSD_MESSAGE_CACHE_OVER_CAPACITY_ERROR,
	LSD_OVER_HDD_CAPACITY_ERROR,
	LSD_UNKNOWN_SERVICE_ERROR
};

class error : public std::exception {
public:
	error(const std::string& format, ...);
	error(enum error_type type, const std::string& format, ...);

	virtual ~error() throw ();

	error(error const& other);
	error& operator = (error const& other);

	static const int MESSAGE_SIZE = 512;

	virtual char const* what() const throw ();

private:
	char message_[MESSAGE_SIZE];
	enum error_type type_;
};

} // namespace lsd

#endif // _LSD_ERROR_HPP_INCLUDED_

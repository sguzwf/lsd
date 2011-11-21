#ifndef _LSD_HANDLE_HPP_INCLUDED_
#define _LSD_HANDLE_HPP_INCLUDED_

#include <string>
#include <map>

#include "handle_info.hpp"

namespace lsd {

template <typename LSD_T>
class handle {
public:
	handle(const handle_info<LSD_T>& info) : info_(info) {};

	handle_info<LSD_T> info_;
};

} // namespace lsd

#endif // _LSD_HANDLE_HPP_INCLUDED_

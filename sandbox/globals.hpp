#ifndef _LSD_GLOBALS_HPP_INCLUDED_
#define _LSD_GLOBALS_HPP_INCLUDED_

#include "structs.hpp"
#include "host_info.hpp"
#include "service.hpp"
#include "service_info.hpp"
#include "handle.hpp"
#include "handle_info.hpp"

namespace lsd {

// main types definition
typedef lsd_types LT;

// host info
typedef host_info<LT> host_info_t;

// service info
typedef service_info<LT> service_info_t;
typedef service<LT> service_t;

// handle info
typedef handle_info<LT> handle_info_t;
typedef handle<LT> handle_t;

} // namespace lsd

#endif // _LSD_GLOBALS_HPP_INCLUDED_

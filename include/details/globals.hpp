//
// Copyright (C) 2011 Rim Zaidullin <creator@bash.org.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef _LSD_GLOBALS_HPP_INCLUDED_
#define _LSD_GLOBALS_HPP_INCLUDED_

#include "lsd/structs.hpp"
#include "details/host_info.hpp"
#include "details/service.hpp"
#include "details/service_info.hpp"
#include "details/handle.hpp"
#include "details/handle_info.hpp"

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

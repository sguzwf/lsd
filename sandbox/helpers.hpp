#ifndef _LSD_HELPERS_HPP_INCLUDED_
#define _LSD_HELPERS_HPP_INCLUDED_

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

namespace lsd {

bool get_hostname_and_ip(std::string& hostname, std::string& ip);

} // namespace lsd

#endif // _LSD_HELPERS_HPP_INCLUDED_
#ifndef _PQM_HELPERS_HPP_INCLUDED_
#define _PQM_HELPERS_HPP_INCLUDED_

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

namespace pmq {

bool get_hostname_and_ip(std::string& hostname, std::string& ip);

} // namespace pmq

#endif // _PQM_HELPERS_HPP_INCLUDED_
#include "details/helpers.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace pmq {

bool
get_hostname_and_ip(std::string& hostname, std::string& ip) {
	char name[256];
	int res = gethostname(name, sizeof(char) * 256);

	if (res != 0) {
		return false;
	}
	
	hostname = name;
	
	hostent* record = gethostbyname(name);
	if (!record) {
		return false;
	}
	
	in_addr* address = (in_addr*)(record->h_addr);
	
	if (!address) {
		return false;
	}
	
	ip = inet_ntoa(*address);
	
	return true;
}

} // namespace pmq

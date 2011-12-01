#ifndef _LSD_CURL_HOSTS_FETCHER_HPP_INCLUDED_
#define _LSD_CURL_HOSTS_FETCHER_HPP_INCLUDED_

#include <memory>
#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/cstdint.hpp>
#include <curl/curl.h>

#include "refresher.hpp"
#include "host_info.hpp"
#include "service_info.hpp"

namespace lsd {

class curl_hosts_fetcher : private boost::noncopyable  {
public:
	curl_hosts_fetcher(const std::string& url, boost::uint32_t interval, service_info_t service_info);
	virtual ~curl_hosts_fetcher();
	
	void start();
	void stop();
	
	// passes list of hosts of specific service to callback
	void set_callback(boost::function<void(std::vector<host_info_t>&, service_info_t)> callback);

private:
	void interval_func();
	static int curl_writer(char* data, size_t size, size_t nmemb, std::string* buffer_in);

private:
	CURL* curl_;
	boost::function<void(std::vector<host_info_t>&, service_info_t)> callback_;
	std::string url_;
	boost::uint32_t interval_;
	std::auto_ptr<refresher> refresher_;
	service_info_t service_info_;
};

} // namespace lsd

#endif // _LSD_CURL_HOSTS_FETCHER_HPP_INCLUDED_

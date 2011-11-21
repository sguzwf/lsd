#include "settings.h"

#include <stdexcept>
#include <iostream>

#include <boost/current_function.hpp>
#include <boost/tokenizer.hpp>

#include "curl_hosts_fetcher.hpp"

namespace lsd {

curl_hosts_fetcher::curl_hosts_fetcher(const std::string& url, boost::uint32_t interval, service_info_t service_info) :
	curl_(NULL),
	url_(url),
	interval_(interval),
	service_info_(service_info)
{
	curl_ = curl_easy_init();
	start();
}

curl_hosts_fetcher::~curl_hosts_fetcher() {
	stop();
	curl_easy_cleanup(curl_);
}

// passes list of hosts to callback
void
curl_hosts_fetcher::set_callback(boost::function<void(std::vector<host_info_t>&, service_info_t)> callback) {
	callback_ = callback;
}

void
curl_hosts_fetcher::start() {
	refresher_.reset(new refresher(boost::bind(&curl_hosts_fetcher::interval_func, this), interval_));
}

void
curl_hosts_fetcher::stop() {
	refresher_.reset(NULL);
}

int
curl_hosts_fetcher::curl_writer(char* data, size_t size, size_t nmemb, std::string* buffer_in) {
	if (buffer_in != NULL) {
		buffer_in->append(data, size * nmemb);
		return size * nmemb;
	}

	return 0;
}

void
curl_hosts_fetcher::interval_func() {
	CURLcode result = CURLE_OK;
	char error_buffer[CURL_ERROR_SIZE];
	std::string buffer;
	
	if (curl_) {
		curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error_buffer);
		curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
		curl_easy_setopt(curl_, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, curl_writer);
		curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer);

		// Attempt to retrieve the remote page
		result = curl_easy_perform(curl_);
	}
	
	if (CURLE_OK != result) {
		return;
	}
	
	long response_code = 0;
	result = curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
	
	if (CURLE_OK != result || response_code != 200) {
		return;
	}
	
	// get hosts from received data
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\n");
	tokenizer tokens(buffer, sep);
	
	std::vector<host_info_t> hosts;
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
		try {
			host_info_t host(*tok_iter);
			hosts.push_back(host);
		}
		catch (...) {
		}
	}
	
	callback_(hosts, service_info_);
}

} // namespace lsd
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

#ifndef _LSD_STATISTICS_COLLECTOR_HPP_INCLUDED_
#define _LSD_STATISTICS_COLLECTOR_HPP_INCLUDED_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>

#include "details/configuration.hpp"

namespace lsd {

class statistics_collector : private boost::noncopyable {
public:
	statistics_collector(boost::shared_ptr<configuration> config,
						 boost::shared_ptr<zmq::context_t> context);

	statistics_collector(boost::shared_ptr<configuration> config,
						 boost::shared_ptr<zmq::context_t> context,
						 boost::shared_ptr<base_logger> logger);

	virtual ~statistics_collector();
	
	void set_logger(boost::shared_ptr<base_logger> logger);
	void enable(bool value);
	std::string as_json() const;

private:
	void init();
	void process_remote_connection();
	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config() const;

private:
	bool is_enabled_;

	// global configuration object
	boost::shared_ptr<configuration> config_;

	// logger
	boost::shared_ptr<base_logger> logger_;

	// zmq context
	boost::shared_ptr<zmq::context_t> zmq_context_;

	boost::thread thread_;
	boost::mutex mutex_;
	bool is_running_;
};

} // namespace lsd

#endif // _LSD_CACHED_MESSAGE_HPP_INCLUDED_

#include "settings.h"

#include <iostream>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "client.hpp"

/*
#include "progress_timer.hpp"
#include "smart_logger.hpp"
#include "curl_hosts_fetcher.hpp"
#include "globals.hpp"
#include "http_heartbeats_collector.hpp"
#include "configuration.hpp"
#include "context.hpp"
*/

namespace po = boost::program_options;

std::string config_path = "/home/rimz/lsd/sandbox/config.json";

/*
void callback(const lsd::service_info_t& s_info, const std::vector<lsd::host_info_t>& hosts, const std::vector<lsd::handle_info_t>& handles) {
	std::cout << "--- service ---\n";
	std::cout << "name: " << s_info.name_ << "\n";
	std::cout << "description: " << s_info.description_ << "\n";
	std::cout << "application name: " << s_info.app_name_ << "\n";
	std::cout << "hosts url: " << s_info.hosts_url_ << "\n";
	std::cout << "control port: " << s_info.control_port_ << "\n";

	std::cout << "hosts:\n";
	for (size_t i = 0; i < hosts.size(); ++i) {
		std::cout << "\t" << i + 1 << ") " << hosts[i] << "\n";		
	}

	std::cout << "handles:\n";
	for (size_t i = 0; i < handles.size(); ++i) {
		std::cout << "\t" << i + 1 << ") " << handles[i] << "\n";	
	}
}
*/

void create_client(int add_messages_count) {
	lsd::client c(config_path);
	c.connect();
	sleep(60);
	add_messages_count++;
	/*
	lsd::context sctx(config_path);
	lsd::http_heartbeats_collector collector(sctx.config(), sctx.zmq_context());
	collector.set_callback(&callback);
	collector.set_logger(sctx.logger());
	collector.run();
	sleep(10);
	*/

	//
	/*
	

	lsd::server s(config_path);
	s.connect();

	if (add_messages_count > 0) {
		lsd::progress_timer t;
		for (int i = 0; i < add_messages_count; ++i) {		
			std::string message = "[ \"message " + boost::lexical_cast<std::string>(i) + "\" ]";
			s.send_message(message, "zbr");
		}

		lsd::smart_logger<>::log_common("added %d messages in %.4f seconds", add_messages_count, t.elapsed());
	}

	sleep(120);
	*/
}

int
main(int argc, char** argv) {	
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "Produce help message")
			("client,c", "Start as server")
			("messages,m", po::value<int>(), "Add messages to server")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if (vm.count("client")) {
			int add_messages_count = vm.count("messages") ? vm["messages"].as<int>() : 0;
			create_client(add_messages_count);

			return EXIT_SUCCESS;
		}

		std::cout << desc << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 main.cpp multicast_client.cpp multicast_server.cpp \
    helpers.cpp client.cpp heartbeat_notifier.cpp refresher.cpp \
    multicast_heartbeats_collector.cpp http_heartbeats_collector.cpp \
    server.cpp server_impl.cpp server_socket.cpp persistent_storage.cpp \
    persistent_storage_callback.cpp server_context.cpp progress_timer.cpp \
    json/json_reader.cpp json/json_value.cpp json/json_writer.cpp \
    cached_message.cpp configuration.cpp client_context.cpp client_impl.cpp \
    curl_hosts_fetcher.cpp
 */

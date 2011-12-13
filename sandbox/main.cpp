#include "settings.h"

#include <iostream>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <msgpack.hpp>

#include "client.hpp"

namespace po = boost::program_options;

std::string config_path = "/home/rimz/lsd/sandbox/config.json";

void create_client(int add_messages_count) {
	lsd::client c(config_path);
	c.connect();

	// create message path
	lsd::message_path path;
	path.service_name = "karma-engine-testing";
	path.handle_name = "event";

	// create message policy
	lsd::message_policy policy;
	policy.deadline = 2.0;

	// create message data
	std::map<std::string, int> event;
	event["service"] = 1;
	event["uid"] = 12345;
	event["score"] = 500;

	msgpack::sbuffer buffer;
	msgpack::pack(buffer, event);

	// send messages
	for (int i = 0; i < add_messages_count; ++i) {
		std::string uuid1 = c.send_message(buffer.data(), buffer.size(), path, policy);
	}

	sleep(30);
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


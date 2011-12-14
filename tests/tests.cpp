#define BOOST_AUTO_TEST_MAIN

#include <boost/mpl/list.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/thread.hpp>

#include <msgpack.hpp>

#include "lsd/client.hpp"
#include "details/time_value.hpp"

typedef boost::mpl::list<int, long, unsigned char> test_types;

BOOST_AUTO_TEST_SUITE(test_cached_read);

void spawn_client() {
	std::string config_path = "config.json";
	int add_messages_count = 300;

	lsd::client client(config_path);
	client.connect();

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
		std::string uuid1 = client.send_message(buffer.data(), buffer.size(), path, policy);
	}

	sleep(30);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(first_test, T, test_types) {
	//spawn_client();
	BOOST_CHECK_EQUAL(1 == 1, true);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(time_value_test1, T, test_types) {
	lsd::time_value tv;
	BOOST_CHECK_EQUAL(tv.as_double(), 0.0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(time_value_test2, T, test_types) {
	lsd::time_value tv, tv2;
	tv.init_from_current_time();
	tv2 = tv;

	BOOST_CHECK_EQUAL(tv == tv2, true);
	BOOST_CHECK_EQUAL(tv != tv2, false);
}

BOOST_AUTO_TEST_SUITE_END();

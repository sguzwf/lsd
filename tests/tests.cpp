#define BOOST_AUTO_TEST_MAIN

#include <boost/mpl/list.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/thread.hpp>

#include "lsd/client.hpp"

typedef boost::mpl::list<int, long, unsigned char> test_types;

//static phd::phone_detector detector("phonedetect.bin", false);

BOOST_AUTO_TEST_SUITE(test_cached_read);

BOOST_AUTO_TEST_CASE_TEMPLATE(first_test, T, test_types) {
	BOOST_CHECK_EQUAL(1 == 1, true);
}

BOOST_AUTO_TEST_SUITE_END();

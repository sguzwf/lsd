bin_PROGRAMS = lsd-sandbox

lsd_sandbox_SOURCES = main.cpp client.cpp client_impl.cpp context.cpp configuration.cpp \
	json/json_reader.cpp json/json_value.cpp json/json_writer.cpp refresher.cpp \
	http_heartbeats_collector.cpp curl_hosts_fetcher.cpp progress_timer.cpp \
	data_container.cpp cached_message.cpp error.cpp message_cache.cpp cached_response.cpp

AM_CPPFLAGS = -I @top_srcdir@/config/ -I @top_srcdir@/include/ @BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_PROGRAM_OPTIONS_LDFLAGS@ @BOOST_LDFLAGS@ @BOOST_THREAD_LDFLAGS@ \
	@OPENSSL_LIBS@ @OPENSSL_LDFLAGS@ -L/usr/local/lib

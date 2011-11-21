lib_LTLIBRARIES = libpmq.la

libpmq_la_SOURCES = multicast_client.cpp multicast_server.cpp \
    helpers.cpp client.cpp heartbeat_notifier.cpp refresher.cpp \
    multicast_heartbeats_collector.cpp conductor_heartbeats_collector.cpp \
    server.cpp server_impl.cpp server_socket.cpp persistent_storage.cpp \
    persistent_storage_callback.cpp server_context.cpp progress_timer.cpp \
    json/json_reader.cpp json/json_value.cpp json/json_writer.cpp \
    cached_message.cpp configuration.cpp client_context.cpp client_impl.cpp

AM_CPPFLAGS = -I @top_srcdir@/config/ -I @top_srcdir@/include/ @BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_PROGRAM_OPTIONS_LDFLAGS@ @BOOST_LDFLAGS@ @BOOST_THREAD_LDFLAGS@ -L/usr/local/lib

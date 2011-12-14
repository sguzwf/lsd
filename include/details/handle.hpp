#ifndef _LSD_HANDLE_HPP_INCLUDED_
#define _LSD_HANDLE_HPP_INCLUDED_

#include <string>
#include <map>
#include <memory>
#include <cerrno>

#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>

#include "json/json.h"

#include "lsd/structs.hpp"

#include "details/error.hpp"
#include "details/context.hpp"
#include "details/handle_info.hpp"
#include "details/host_info.hpp"
#include "details/cached_message.hpp"
#include "details/cached_response.hpp"
#include "details/message_cache.hpp"
#include "details/progress_timer.hpp"

namespace lsd {

#define CONTROL_MESSAGE_NOTHING 0
#define CONTROL_MESSAGE_CONNECT 1
#define CONTROL_MESSAGE_RECONNECT 2
#define CONTROL_MESSAGE_DISCONNECT 3
#define CONTROL_MESSAGE_CONNECT_NEW_HOSTS 4
#define CONTROL_MESSAGE_KILL 5

// predeclaration
template <typename LSD_T> class handle;
typedef handle<LT> handle_t;

template <typename LSD_T>
class handle : public boost::noncopyable {
public:
	typedef std::vector<host_info<LSD_T> > hosts_info_list_t;
	typedef boost::shared_ptr<zmq::socket_t> socket_ptr_t;

	typedef boost::shared_ptr<cached_response> cached_response_prt_t;
	typedef boost::function<void(cached_response_prt_t)> responce_callback_t;

public:
	handle(const handle_info<LSD_T>& info,
		   boost::shared_ptr<lsd::context> context,
		   const hosts_info_list_t& hosts);

	virtual ~handle();

	const handle_info<LSD_T>& info() const;
	boost::shared_ptr<message_cache> messages_cache();

	void connect();
	void connect(const hosts_info_list_t& hosts);
	void connect_new_hosts(const hosts_info_list_t& hosts);
	void reconnect(const hosts_info_list_t& hosts);
	void disconnect();

	void set_responce_callback(responce_callback_t callback);
	void enqueue_message(boost::shared_ptr<cached_message> message);

private:
	void kill();
	void dispatch_messages();
	void log_dispatch_start();

	// working with control messages
	void establish_control_conection(socket_ptr_t& control_socket);
	int receive_control_messages(socket_ptr_t& control_socket);
	void dispatch_control_messages(int type, socket_ptr_t& main_socket);

	// working with messages
	void dispatch_next_available_message(socket_ptr_t main_socket);
	void connect_zmq_socket_to_hosts(socket_ptr_t& socket,
									 hosts_info_list_t& hosts);

	bool check_for_responses(socket_ptr_t& main_socket);
	void dispatch_responces(socket_ptr_t& main_socket);

	boost::shared_ptr<base_logger> logger();
	boost::shared_ptr<configuration> config();
	boost::shared_ptr<lsd::context> context();

private:
	handle_info<LSD_T> info_;
	boost::shared_ptr<lsd::context> context_;
	hosts_info_list_t hosts_;
	hosts_info_list_t new_hosts_;
	boost::shared_ptr<message_cache> message_cache_;

	boost::thread thread_;
	boost::mutex mutex_;
	volatile bool is_running_;
	volatile bool is_connected_;

	std::auto_ptr<zmq::socket_t> zmq_control_socket_;
	bool receiving_control_socket_ok_;

	responce_callback_t response_callback_;
};

template <typename LSD_T>
handle<LSD_T>::handle(const handle_info<LSD_T>& info,
					  boost::shared_ptr<lsd::context> lsd_context,
					  const hosts_info_list_t& hosts) :
	info_(info),
	context_(lsd_context),
	hosts_(hosts),
	is_running_(false),
	is_connected_(false),
	receiving_control_socket_ok_(false)
{
	logger()->log(PLOG_DEBUG, "created service %s handle %s", info.service_name_.c_str(), info.name_.c_str());

	// create message cache
	message_cache_.reset(new message_cache(context(), config()->message_cache_type()));

	// create control socket
	std::string conn_str = "inproc://service_control_" + info_.service_name_ + "_" + info_.name_;
	zmq_control_socket_.reset(new zmq::socket_t(*(context()->zmq_context()), ZMQ_PAIR));
	zmq_control_socket_->bind(conn_str.c_str());

	// run main thread
	is_running_ = true;
	thread_ = boost::thread(&handle<LSD_T>::dispatch_messages, this);
}

template <typename LSD_T>
handle<LSD_T>::~handle() {
	kill();

	zmq_control_socket_->close();
	zmq_control_socket_.reset(NULL);

	thread_.join();
}

template <typename LSD_T> void
handle<LSD_T>::dispatch_messages() {
	// establish connections
	socket_ptr_t main_socket;
	socket_ptr_t control_socket;

	establish_control_conection(control_socket);
	log_dispatch_start();

	// process messages
	while (is_running_) {

		// receive control message
		int control_message = receive_control_messages(control_socket);

		// received kill message, finalize everything
		if (control_message == CONTROL_MESSAGE_KILL) {
			logger()->log(PLOG_DEBUG, "CONTROL_MESSAGE_KILL");
			is_running_ = false;
			break;
		}

		// process incoming control messages
		if (control_message > 0) {
			dispatch_control_messages(control_message, main_socket);

			timeval tv;
			gettimeofday(&tv, NULL);

			while (true) {
				if (progress_timer::elapsed_from_time(&tv) > 1.000) {
					break;
				}
			}
		}

		// send new message if any
		if (is_running_ && is_connected_) {
			dispatch_next_available_message(main_socket);
		}

		// check for timed out messages
		if (is_connected_ && is_running_) {
			static bool started = false;
			static timeval tv;
			if (!started) {
				gettimeofday(&tv, NULL);
				started = true;
			}

			if (progress_timer::elapsed_from_time(&tv) > 1.0) {
				gettimeofday(&tv, NULL);
				messages_cache()->process_timed_out_messages();
			}
		}

		/*
		// check for message responces
		bool received_response = false;
		if (is_connected_ && is_running_) {
			received_response = check_for_responses(main_socket);
		}

		// process received responce(s)
		if (is_connected_ && is_running_ && received_response) {
			dispatch_responces(main_socket);
		}
		*/
	}

	control_socket.reset();
	main_socket.reset();
}

template <typename LSD_T> void
handle<LSD_T>::establish_control_conection(socket_ptr_t& control_socket) {
	control_socket.reset(new zmq::socket_t(*(context()->zmq_context()), ZMQ_PAIR));

	if (control_socket.get()) {
		std::string conn_str = "inproc://service_control_" + info_.service_name_ + "_" + info_.name_;
		control_socket->connect(conn_str.c_str());
		receiving_control_socket_ok_ = true;
	}
}

template <typename LSD_T> int
handle<LSD_T>::receive_control_messages(socket_ptr_t& control_socket) {
	if (!is_running_) {
		return 0;
	}

	// poll for responce
	zmq_pollitem_t poll_items[1];
	poll_items[0].socket = *control_socket;
	poll_items[0].fd = 0;
	poll_items[0].events = ZMQ_POLLIN;
	poll_items[0].revents = 0;

	int socket_response = zmq_poll(poll_items, 1, 0);

	if (socket_response <= 0) {
		return 0;
	}

	// in case we received control message
    if ((ZMQ_POLLIN & poll_items[0].revents) == ZMQ_POLLIN) {
    	int received_message = 0;

    	zmq::message_t reply;
    	try {
    		control_socket->recv(&reply);
    		memcpy((void *)&received_message, reply.data(), reply.size());

    		return received_message;
    	}
    	catch (const std::exception& ex) {
    			std::string error_msg = "some very ugly shit happend while recv on socket at ";
    			error_msg += std::string(BOOST_CURRENT_FUNCTION);
    			error_msg += " details: " + std::string(ex.what());
    			throw error(error_msg);
    	}
    }

    return 0;
}

template <typename LSD_T> void
handle<LSD_T>::dispatch_control_messages(int type, socket_ptr_t& main_socket) {
	if (!is_running_) {
		return;
	}

	switch (type) {
		case CONTROL_MESSAGE_CONNECT:
			logger()->log(PLOG_DEBUG, "CONTROL_MESSAGE_CONNECT");

			// create new main socket in case we're not connected
			if (!is_connected_) {
				main_socket.reset(new zmq::socket_t(*(context()->zmq_context()), ZMQ_DEALER));
				connect_zmq_socket_to_hosts(main_socket, hosts_);
				is_connected_ = true;
			}
			break;

		case CONTROL_MESSAGE_RECONNECT:
			logger()->log(PLOG_DEBUG, "CONTROL_MESSAGE_RECONNECT");

			// kill old socket, create new
			main_socket.reset(new zmq::socket_t(*(context()->zmq_context()), ZMQ_DEALER));
			connect_zmq_socket_to_hosts(main_socket, hosts_);
			is_connected_ = true;
			break;

		case CONTROL_MESSAGE_DISCONNECT:
			logger()->log(PLOG_DEBUG, "CONTROL_MESSAGE_DISCONNECT");

			// kill socket
			main_socket.reset();
			is_connected_ = false;
			break;

		case CONTROL_MESSAGE_CONNECT_NEW_HOSTS:
			logger()->log(PLOG_DEBUG, "CONTROL_MESSAGE_CONNECT_NEW_HOSTS");

			// connect socket to new hosts
			hosts_info_list_t new_hosts;
			if (is_connected_ && !new_hosts_.empty()) {
				new_hosts = new_hosts_;
				new_hosts_.clear();

				if (!new_hosts.empty()) {
					connect_zmq_socket_to_hosts(main_socket, new_hosts);
				}
			}
			break;
	}
}

template <typename LSD_T> void
handle<LSD_T>::connect_zmq_socket_to_hosts(socket_ptr_t& socket,
										   hosts_info_list_t& hosts)
{
	if (hosts.empty()) {
		return;
	}

	if (!socket.get()) {
		std::string error_msg = "service: " + info_.service_name_;
		error_msg += ", handle: " + info_.name_ + " — socket object is empty";
		error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}

	// connect socket to hosts
	std::string connection_str;
	try {
		for (size_t i = 0; i < hosts.size(); ++i) {
			std::string port = boost::lexical_cast<std::string>(info_.port_);
			std::string ip = host_info<LSD_T>::string_from_ip(hosts[i].ip_);
			connection_str = "tcp://" + ip + ":" + port;
			logger()->log(PLOG_DEBUG, "handle connection str: %s", connection_str.c_str());
			//int timeout = 100;
			//socket->setsockopt(ZMQ_LINGER, &timeout, sizeof(timeout));
			//std::string ident = info_.service_name_ + info_.name_;
			//socket->setsockopt(ZMQ_IDENTITY, ident.c_str(), ident.length());
			socket->connect(connection_str.c_str());
		}
	}
	catch (const std::exception& ex) {
		std::string error_msg = "service: " + info_.service_name_;
		error_msg += ", handle: " + info_.name_ + " — could not connect to ";
		error_msg += connection_str + " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}
}

template <typename LSD_T> void
handle<LSD_T>::dispatch_next_available_message(socket_ptr_t main_socket) {
	// validate socket
	if (!main_socket) {
		std::string error_msg = "service: " + info_.service_name_;
		error_msg += ", handle: " + info_.name_ + " — empty socket object";
		error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}

	// send new message if any
	if (messages_cache()->new_messages_count() == 0) {
		return;
	}

	try {
		cached_message& new_msg = messages_cache()->get_new_message();

		// send header
		zmq::message_t empty_message(0);
		if (true != main_socket->send(empty_message, ZMQ_SNDMORE)) {
			return;
		}

		// send envelope
		std::string msg_header = new_msg.json();
		size_t header_size = msg_header.length();
		zmq::message_t header(header_size);
		memcpy((void *)header.data(), msg_header.c_str(), header_size);

		if (true != main_socket->send(header, ZMQ_SNDMORE)) {
			return;
		}

		// send data
		size_t data_size = new_msg.data().size();
		zmq::message_t message(data_size);

		if (data_size > 0) {
			memcpy((void *)message.data(), new_msg.data().data(), data_size);
		}

		if (true != main_socket->send(message)) {
			return;
		}

		// assign message flags
		new_msg.set_sent(true);
		new_msg.set_sent_timestamp(progress_timer::get_precise_time());

		// move message to sent
		messages_cache()->move_new_message_to_sent();
	}
	catch (const std::exception& ex) {
		std::string error_msg = " service: " + info_.service_name_;
		error_msg += ", handle: " + info_.name_ + " — could not send message";
		error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION) + "reason: ";
		error_msg += ex.what();
		throw error(error_msg);
	}
}

template <typename LSD_T> void
handle<LSD_T>::dispatch_responces(socket_ptr_t& main_socket) {
	zmq::message_t reply;

	// receive messages while we have any on the socket
	while (true) {
		std::string json_header;

		try {
			// receive reply header
			if (!main_socket->recv(&reply, ZMQ_NOBLOCK)) {
				logger()->log(PLOG_ERROR, "bad head");
				break;
			}

			// receive json envelope
			if (!main_socket->recv(&reply)) {
				logger()->log(PLOG_ERROR, "bad envelope");
				break;
			}

			// get envelope json string
			json_header = std::string((const char*)reply.data(), reply.size());
			logger()->log(PLOG_DEBUG, "received header: %s", json_header.c_str());

			bool is_response_completed = false;
			std::string uuid;
			Json::Value envelope_val;
			Json::Reader jreader;
			int error_code = 0;
			std::string error_message;

			// parse envelope json
			if (jreader.parse(json_header.c_str(), envelope_val)) {
				uuid = envelope_val.get("uuid", "").asString();
				is_response_completed = envelope_val.get("completed", false).asBool();
				error_code = envelope_val.get("code", 0).asInt();
				error_message = envelope_val.get("message", "").asString();

				if (uuid.empty()) {
					std::string error_msg = "service: " + info_.service_name_;
					error_msg += ", handle: " + info_.name_ + " — uuid is empty in response envelope";
					error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION);
					throw error(error_msg);
				}
			}
			else {
				std::string error_msg = "service: " + info_.service_name_;
				error_msg += ", handle: " + info_.name_ + " — could not parse response envelope";
				error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION);
				throw error(error_msg);
			}

			if (error_code != 0) {
				// get message from sent cache
				cached_message& sent_msg = messages_cache()->get_sent_message(uuid);

				// create response object
				cached_response_prt_t new_response;
				new_response.reset(new cached_response(uuid, sent_msg.path(), error_code, error_message));

				// remove message from cache
				messages_cache()->remove_message_from_cache(uuid);

				// invoke response callback
				response_callback_(new_response);
			}

			/*
			// receive data
			if (!is_response_completed && error_code == 0) {
				logger()->log(PLOG_DEBUG, "responce not completed");
				// receive chunk data
				if (!main_socket->recv(&reply)) {
					logger()->log(PLOG_DEBUG, "bad data");
					receiving_failed = true;
					break;
				}

				msgpack::unpacked msg;
				msgpack::unpack(&msg, (const char*)reply.data(), reply.size());

				msgpack::object obj = msg.get();
				std::stringstream stream;
				stream << obj;
				logger()->log(PLOG_DEBUG, "received data: %s", stream.str().c_str());
			}

			if (error_code != 0) {
				logger()->log(PLOG_DEBUG, "err code");
				messages_cache()->remove_message_from_cache(uuid);

				//response_callback_

				// decrement queue size
				receiving_failed = true;

				logger()->log(PLOG_DEBUG, "header: %s", json_header.c_str());
				++err_responces_count;
			}
			else {
				if (is_response_completed) {
					logger()->log(PLOG_DEBUG, "responce completed");
					messages_cache()->remove_message_from_cache(uuid);
					// decrement queue size
					++responces_count;
				}
			}
			*/
		}
		catch (const std::exception& ex) {
			std::string error_msg = "service: " + info_.service_name_;
			error_msg += ", handle: " + info_.name_ + " — error while receiving response";
			error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION) + "reason: ";
			error_msg += ex.what();
			logger()->log(PLOG_DEBUG, error_msg);
			break;
		}
	}

	// increment queue size
	// RESPONSE
	// uuid, path, data, size

	// lsd service, RESPONSE
}

template <typename LSD_T> bool
handle<LSD_T>::check_for_responses(socket_ptr_t& main_socket) {
	if (!is_running_) {
		return 0;
	}

	// validate socket
	if (!main_socket.get()) {
		std::string error_msg = "service: " + info_.service_name_;
		error_msg += ", handle: " + info_.name_ + " — empty socket object";
		error_msg += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_msg);
	}

	// poll for responce
	zmq_pollitem_t poll_items[1];
	poll_items[0].socket = *main_socket;
	poll_items[0].fd = 0;
	poll_items[0].events = ZMQ_POLLIN;
	poll_items[0].revents = 0;

	int socket_response = zmq_poll(poll_items, 1, 0);

	if (socket_response <= 0) {
		return false;
	}

	// in case we received message response
	if ((ZMQ_POLLIN & poll_items[0].revents) == ZMQ_POLLIN) {
		return true;
	}

    return false;
}

template <typename LSD_T> void
handle<LSD_T>::log_dispatch_start() {
	static bool started = false;

	if (!started) {
		std::string format = "thread started for service: %s, handle: %s";
		logger()->log(PLOG_DEBUG, format.c_str(), info_.service_name_.c_str(), info_.name_.c_str());
		started = true;
	}
}

template <typename LSD_T> const handle_info<LSD_T>&
handle<LSD_T>::info() const {
	return info_;
}

template <typename LSD_T> void
handle<LSD_T>::kill() {
	logger()->log(PLOG_DEBUG, "kill");

	if (!is_running_) {
		return;
	}

	// kill dispatch thread from the inside
	int control_message = CONTROL_MESSAGE_KILL;
	zmq::message_t message(sizeof(int));
	memcpy((void *)message.data(), &control_message, sizeof(int));
	zmq_control_socket_->send(message);
}

template <typename LSD_T> void
handle<LSD_T>::connect() {
	logger()->log(PLOG_DEBUG, "connect");

	if (!is_running_ || hosts_.empty() || is_connected_) {
		return;
	}

	// connect to hosts
	int control_message = CONTROL_MESSAGE_CONNECT;
	zmq::message_t message(sizeof(int));
	memcpy((void *)message.data(), &control_message, sizeof(int));
	zmq_control_socket_->send(message);
}

template <typename LSD_T> void
handle<LSD_T>::connect(const hosts_info_list_t& hosts) {
	logger()->log(PLOG_DEBUG, "connect with hosts");

	// no hosts to connect to
	if (!is_running_ || is_connected_ || hosts.empty()) {
		return;
	}
	else {
		// store new hosts
		boost::mutex::scoped_lock lock(mutex_);
		hosts_ = hosts;
	}

	// connect to hosts
	connect();
}

template <typename LSD_T> void
handle<LSD_T>::connect_new_hosts(const hosts_info_list_t& hosts) {
	logger()->log(PLOG_DEBUG, "connect with new hosts");

	// no new hosts to connect to
	if (!is_running_ || hosts.empty()) {
		return;
	}
	else {
		// append new hosts
		boost::mutex::scoped_lock lock(mutex_);
		new_hosts_.insert(new_hosts_.end(), hosts.begin(), hosts.end());
	}

	// connect to new hosts
	int control_message = CONTROL_MESSAGE_CONNECT_NEW_HOSTS;
	zmq::message_t message(sizeof(int));
	memcpy((void *)message.data(), &control_message, sizeof(int));
	zmq_control_socket_->send(message);
}

template <typename LSD_T> void
handle<LSD_T>::reconnect(const hosts_info_list_t& hosts) {
	logger()->log(PLOG_DEBUG, "reconnect");

	// no new hosts to connect to
	if (!is_running_ || hosts.empty()) {
		return;
	}
	else {
		// replace hosts with new hosts
		boost::mutex::scoped_lock lock(mutex_);
		hosts_ = hosts;
	}

	// reconnect to hosts
	int control_message = CONTROL_MESSAGE_RECONNECT;
	zmq::message_t message(sizeof(int));
	memcpy((void *)message.data(), &control_message, sizeof(int));
	zmq_control_socket_->send(message);
}

template <typename LSD_T> void
handle<LSD_T>::disconnect() {
	boost::mutex::scoped_lock lock(mutex_);
	logger()->log(PLOG_DEBUG, "disconnect");

	if (!is_running_) {
		return;
	}

	// disconnect from all hosts
	std::string control_message = boost::lexical_cast<std::string>(CONTROL_MESSAGE_DISCONNECT);
	zmq::message_t message(control_message.length());
	memcpy((void *)message.data(), control_message.c_str(), control_message.length());
	zmq_control_socket_->send(message);
}

template <typename LSD_T> boost::shared_ptr<message_cache>
handle<LSD_T>::messages_cache() {
	if (!message_cache_) {
		std::string error_str = "messages cache object is empty for service: ";
		error_str += info_.service_name_ + ", handle: " + info_.name_;
		error_str += " at " + std::string(BOOST_CURRENT_FUNCTION);
		throw error(error_str);
	}

	return message_cache_;
}

template <typename LSD_T> void
handle<LSD_T>::set_responce_callback(responce_callback_t callback) {
	boost::mutex::scoped_lock lock(mutex_);
	response_callback_ = callback;
}

template <typename LSD_T> void
handle<LSD_T>::enqueue_message(boost::shared_ptr<cached_message> message) {
	boost::mutex::scoped_lock lock(mutex_);
	messages_cache()->enqueue(message);
}

template <typename LSD_T> boost::shared_ptr<lsd::context>
handle<LSD_T>::context() {
	if (!context_) {
		throw error("lsd context object is empty at " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return context_;
}

template <typename LSD_T> boost::shared_ptr<base_logger>
handle<LSD_T>::logger() {
	return context()->logger();
}

template <typename LSD_T> boost::shared_ptr<configuration>
handle<LSD_T>::config() {
	boost::shared_ptr<configuration> conf = context()->config();
	if (!conf.get()) {
		throw error("configuration object is empty at: " + std::string(BOOST_CURRENT_FUNCTION));
	}

	return conf;
}

} // namespace lsd

#endif // _LSD_HANDLE_HPP_INCLUDED_
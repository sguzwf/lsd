#include "settings.h"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "details/client_impl.hpp"
#include "details/structs.hpp"
#include "details/cached_message.hpp"
#include "details/progress_timer.hpp"

namespace pmq {

client_impl::client_impl(const std::string& service_prefix, const std::string& config_path) : is_running_(false) {
	context_.reset(new client_context(service_prefix, config_path));
	logger()->log(PLOG_INFO, "client for service " + service_prefix + " created.");
}

client_impl::~client_impl() {
	heartbeat_notifier_.reset(NULL);

	is_running_ = false;
	thread_.join();
}

void
client_impl::connect() {
	configuration& conf = context_->config();
			
	if (conf.autodiscovery_type() == AT_MULTICAST) {
		configure_heartbeat(context_->service_inf().prefix_, conf.multicast_ip(), conf.multicast_port());
	}
	else if (conf.autodiscovery_type() == AT_CONDUCTOR) {
	}

	is_running_ = true;
	thread_ = boost::thread(&client_impl::run, this);
}

void
client_impl::run() {
	zmq_socket_.reset(new zmq::socket_t(*(context_->zmq_context_.get()), ZMQ_REP));
	
	int64_t hwm = 2;
	zmq_socket_->setsockopt(ZMQ_HWM, &hwm, sizeof(hwm)); 
	
	unsigned short port = context_->service_inf().messages_port_;
	std::string connection_string = "tcp://*:" + boost::lexical_cast<std::string>(port);
	zmq_socket_->bind(connection_string.c_str());
	
	logger()->log(PLOG_DEBUG, "sockets connected.");
	
	while (is_running_) {		
		// poll for responce
		zmq_pollitem_t poll_item;
		poll_item.socket = *zmq_socket_;
		poll_item.fd = 0;
		poll_item.events = ZMQ_POLLIN;
		poll_item.revents = 0;
	
		int res = 0;
		while (res <= 0 && is_running_) {
			res = zmq_poll(&poll_item, 1, context_->config().socket_poll_timeout());
		}

		if (!is_running_) {
			break;
		}
		
		zmq::message_t message;
		zmq_socket_->recv(&message);
        std::string message_str(static_cast<char*>(message.data()), message.size());
		logger()->log(PLOG_DEBUG, "received message: %s", message_str.c_str());
		
		cached_message m;
		
		try {
			m = cached_message::from_json_string(message_str);
			logger()->log("received message: %s", m.message().c_str());
		}
		catch (const std::exception& ex) {
			logger()->log(PLOG_ERROR, "received message: %s has bad json: %s", m.uuid().c_str(), m.message().c_str());
			logger()->log(PLOG_ERROR, "exception msg: %s", ex.what());
		}

        //  Do some 'work'
        boost::posix_time::seconds workTime(1);
		boost::this_thread::sleep(workTime);

        //  Send response back to server
		logger()->log("sending...");
		zmq::message_t reply(m.uuid().length());
		memcpy((void*)reply.data(), m.uuid().c_str(), m.uuid().length());
		zmq_socket_->send(reply);
		logger()->log(PLOG_DEBUG, "sent reply %s", m.uuid().c_str());
    }
	
	zmq_socket_->close();
	zmq_socket_.reset(NULL);
}

void
client_impl::configure_heartbeat(const std::string& service_id, const std::string& ip, unsigned short port) {
	heartbeat_notifier_.reset(new heartbeat_notifier(service_id, ip, port));
	//heartbeat_notifier_->set_logger(logger());
}

boost::shared_ptr<base_logger>
client_impl::logger() {
	return context_->logger();
}

} // namespace pmq
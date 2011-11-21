#include "settings.h"

#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "server_socket.hpp"
#include "progress_timer.hpp"

namespace lsd {

server_socket::server_socket(const service_inf& sinfo, const std::vector<host_info>& hosts, boost::shared_ptr<server_context> context) :
	service_info_(sinfo),
	hosts_(hosts),
	context_(context),
	receiving_control_socket_ok_(false),
	is_running_(false)
{
	output_hosts();
	connect();
}

server_socket::~server_socket() {
	disconnect();
}

void
server_socket::connect() {
	zmq_control_socket_.reset(new zmq::socket_t(*(context_->zmq_context()), ZMQ_PAIR));
	zmq_control_socket_->bind(("inproc://service_control_" + service_info_.prefix_).c_str());
	
	is_running_ = true;
	thread_ = boost::thread(&server_socket::process_message_queue, this);
}

void
server_socket::disconnect() {
	if (!is_running_) {
		return;
	}

	is_running_ = false;
	
	zmq_control_socket_->close();
	zmq_control_socket_.reset(NULL);
	
	thread_.join();
}

void
server_socket::update_socket_hosts(const std::vector<host_info>& hosts) {	
	bool hosts_missing = false;
	
	std::map<std::string, host_info> new_hosts;
	for (size_t i = 0; i < hosts.size(); ++i) {
		new_hosts[hosts[i].ip_] = hosts[i];
	}
		
	// see if new hosts has everything from hosts_ or not
	for (size_t i = 0; i < hosts_.size(); ++i) {
		std::map<std::string, host_info>::iterator it = new_hosts.find(hosts_[i].ip_);
		
		if (it == new_hosts.end()) {
			hosts_missing = true;
			break;
		}
	}

	if (hosts_missing) {
		std::cout << "missing hosts — RECONNECT" << std::endl;
		
		disconnect();

		hosts_.assign(hosts.begin(), hosts.end());
		output_hosts();
	
		// reconnect completely
		connect();
	}
	else {
		std::cout << "new hosts — ADD HOSTS" << std::endl;
		// just connect to new hosts
		std::vector<host_info> added_hosts;
		
		// get list of newly added hosts
		std::map<std::string, host_info> current_hosts;
		for (size_t i = 0; i < hosts_.size(); ++i) {
			current_hosts[hosts_[i].ip_] = hosts_[i];
		}
		
		for (size_t i = 0; i < hosts.size(); ++i) {
			std::map<std::string, host_info>::iterator it = current_hosts.find(hosts[i].ip_);

			if (it == current_hosts.end()) {
				added_hosts.push_back(hosts[i]);
			}
		}

		if (!added_hosts.empty()) {
			connect_new_hosts(added_hosts);
		}
		
		hosts_.assign(hosts.begin(), hosts.end());
		output_hosts();
	}
}

void
server_socket::connect_new_hosts(const std::vector<host_info>& hosts) {
	std::string control_message = "hosts:";
	
	for (size_t i = 0; i < hosts.size(); ++i) {
		control_message += hosts[i].ip_;
		
		if (i < hosts.size() - 1) {
			control_message += ",";
		}
	}

	zmq::message_t message(control_message.length());
	memcpy((void *)message.data(), control_message.c_str(), control_message.length());
	zmq_control_socket_->send(message);
}

void
server_socket::output_hosts() {
 	for (size_t i = 0; i < hosts_.size(); ++i) {
		logger()->log("%d - %s (%s)", i, hosts_[i].name_.c_str(), hosts_[i].ip_.c_str());
	}
}

void
server_socket::process_message_queue() {
	// establish connections
	std::auto_ptr<zmq::socket_t> zmq_socket;
	zmq_socket.reset(new zmq::socket_t(*(context_->zmq_context()), ZMQ_DEALER));
	
	std::auto_ptr<zmq::socket_t> zmq_control_socket2;
	zmq_control_socket2.reset(new zmq::socket_t(*(context_->zmq_context()), ZMQ_PAIR));

	if (zmq_control_socket_.get()) {
		zmq_control_socket2->connect(("inproc://service_control_" + service_info_.prefix_).c_str());
		receiving_control_socket_ok_ = true;
	}
	
	std::string connection_str;
		
	try {
		for (size_t i = 0; i < hosts_.size(); ++i) {
			//std::string port = boost::lexical_cast<std::string>(service_info_.messages_port_); 
			//connection_str = "tcp://" + hosts_[i].ip_ + ":" + port;
			//logger()->log(PLOG_DEBUG, "connection str: %s", connection_str.c_str());
			//int timeout = 0;
			//zmq_socket->setsockopt(ZMQ_LINGER, &timeout, sizeof(timeout));
			//zmq_socket->connect(connection_str.c_str());
		}
	}
	catch (const std::exception& ex) {
		std::string error_msg = "could not connect to ";
		error_msg += connection_str + " at server_socket::connect()";
		throw std::runtime_error(error_msg);
	}
	
	// process messages
	while (is_running_) {
		static bool started = false;
		
		if (!started) {
			logger()->log(PLOG_DEBUG, "thread started");
			logger()->log(PLOG_DEBUG, "service_prefix_: %s", service_info_.prefix_.c_str());

			started = true;
		}
		
		cached_message msg;

		{
			boost::mutex::scoped_lock lock(mutex_);
			if (context_->storage()->pending_message(msg, service_info_.prefix_)) {

				std::string json_msg = msg.json();

				logger()->log(PLOG_DEBUG, "sending message: %s", json_msg.c_str());

				zmq::message_t empty_message(0);
				zmq_socket->send(empty_message, ZMQ_SNDMORE);

				zmq::message_t message(json_msg.length());
				memcpy((void *)message.data(), json_msg.c_str(), json_msg.length());
				zmq_socket->send(message);
				logger()->log(PLOG_DEBUG, "message sent.");
				logger()->log("sent message: %s", msg.message().c_str());

				// mark send message
				msg.set_sent(true);
				msg.set_sent_timestamp(progress_timer::get_precise_time());
				context_->storage()->update_message(msg);
				sent_messages_[msg.uuid()] = msg;
			}
		}

		logger()->log(PLOG_DEBUG, "poll...");

		// poll for responce
		zmq_pollitem_t poll_items[2];
		poll_items[0].socket = *zmq_socket;
		poll_items[0].fd = 0;
		poll_items[0].events = ZMQ_POLLIN;
		poll_items[0].revents = 0;

		poll_items[1].socket = *zmq_control_socket2;
		poll_items[1].fd = 0;
		poll_items[1].events = ZMQ_POLLIN;
		poll_items[1].revents = 0;
		
		int res = 0;
		while (res <= 0 && is_running_) {
			res = zmq_poll(poll_items, 2, DEFAULT_SOCKET_POLL_TIMEOUT);
			
			// cleanup timeouted messages
			bool found_timed_out_messages = false;
			
			if (!sent_messages_.empty()) {
				for (std::map<std::string, cached_message>::iterator it = sent_messages_.begin(); it != sent_messages_.end();) {
					cached_message cm = it->second;
					timeval tv = it->second.sent_timestamp();
					
					if (progress_timer::elapsed_from_time(&tv) > MESSAGE_TIMEOUT) {
						logger()->log(PLOG_DEBUG, "message %s timed out!", cm.uuid().c_str());
						logger()->log("message %s timed out!", cm.message().c_str());
						sent_messages_.erase(it++);
						cm.set_sent(false);
						context_->storage()->update_message(cm);

						found_timed_out_messages = true;
					}
					else {
						++it;
					}
				}
			}
			
			if (found_timed_out_messages) {
				logger()->log(PLOG_DEBUG, "found timed out messages.");
				cached_message msg;
				if (context_->storage()->pending_message(msg, service_info_.prefix_)) {
					logger()->log(PLOG_DEBUG, "message %s still pending", msg.message().c_str());
				}
				
				break;
			}
		}

		if (!is_running_) {
			continue;
		}

		logger()->log(PLOG_DEBUG, "poll res = %d", res);

		if ((ZMQ_POLLIN & poll_items[0].revents) == ZMQ_POLLIN) {
			logger()->log(PLOG_DEBUG, "receiving message.");
			std::string reply_str;
			zmq::message_t reply;

			while (true) {
				bool receiving_failed = false;

				// receive reply
				try {
					if (!zmq_socket->recv(&reply, ZMQ_NOBLOCK)) {
						break;
					}

					zmq_socket->recv(&reply);
					reply_str = std::string(static_cast<char*>(reply.data()), reply.size());
					logger()->log(PLOG_DEBUG, "received reply: %s", reply_str.c_str());
				}
				catch (...) {
					receiving_failed = true;
				}

				if (!receiving_failed) {
					// find sent message by uuid
					std::map<std::string, cached_message>::iterator it = sent_messages_.find(reply_str);

					if (it != sent_messages_.end()) {
						logger()->log("reply is good for message: %s", it->second.message().c_str());

						cached_message sent_msg;
						sent_msg.set_uuid(reply_str);
						sent_msg.set_service_prefix(service_info_.prefix_);
						context_->storage()->remove_message(sent_msg);
						sent_messages_.erase(it);
						logger()->log(PLOG_DEBUG, "message removed");
					}
				}
			}

			logger()->log(PLOG_DEBUG, "receiving message DONE.");
		}

		if ((ZMQ_POLLIN & poll_items[1].revents) == ZMQ_POLLIN) {
			logger()->log(PLOG_DEBUG, "receiving control message.");

			std::string reply_str;
			zmq::message_t reply;
			std::string control_msg_prefix = "hosts:";
			int pr_len = control_msg_prefix.length();

			try {
				logger()->log(PLOG_DEBUG, "receive CONTROL message.");
				zmq_control_socket2->recv(&reply, ZMQ_NOBLOCK);
				reply_str = std::string(static_cast<char*>(reply.data()), reply.size());


				if (!reply_str.empty()) {
					if (reply_str.substr(0, pr_len) == control_msg_prefix) {
						std::string new_hosts_list = reply_str.substr(pr_len, reply_str.length() - pr_len);
						logger()->log(PLOG_DEBUG, "hosts control message: %s", new_hosts_list.c_str());

						typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
						boost::char_separator<char> sep(",");
						tokenizer tokens(new_hosts_list, sep);

						int count = 0;
						for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
							count++;
						}

						for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
							std::string connection_str = "tcp://"+ (*tok_iter) + ":5555";
							logger()->log(PLOG_DEBUG, "NEW connection str: %s", connection_str.c_str());
							zmq_socket->connect(connection_str.c_str());
						}
					}
				}

				logger()->log(PLOG_DEBUG, "receive CONTROL message DONE.");
			}
			catch (...) {
				std::string err_message = "some very ugly shit happend at";
				err_message += __func__;
				err_message += " while zmq_control_socket2->recv";

				throw std::runtime_error(err_message);
			}

			logger()->log(PLOG_DEBUG, "receiving control message DONE.");
		}
	}
	
	// clean-up
	sent_messages_.clear();
	context_->storage()->mark_messages_unsent(service_info_.prefix_);

	// close connections
	receiving_control_socket_ok_ = false;
	zmq_control_socket2->close();
	zmq_control_socket2.reset(NULL);
	
	zmq_socket->close();
	zmq_socket.reset(NULL);
}

void
server_socket::wake_up() {
	if (!is_running_ || !receiving_control_socket_ok_ || !zmq_control_socket_.get()) {
		return;
	}

	zmq::message_t empty_message(0);
	logger()->log(PLOG_DEBUG, "send CONTROL message.");
	zmq_control_socket_->send(empty_message);
	logger()->log(PLOG_DEBUG, "send CONTROL message DONE.");
}

boost::shared_ptr<base_logger>
server_socket::logger() {	
	return context_->logger();
}

} // namespace lsd
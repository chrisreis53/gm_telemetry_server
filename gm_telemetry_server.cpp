/*
 * Copyright 2007-2016 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */



/**
 * @file gmsub.cpp
 *
 * A C++ example demonstration of GMSEC publish/subscribe functionality.
 * The associated example gmsub.cpp will publish a message and this program
 * will subscribe and listen for it.
 * gmsub should be run before gmpub.
 *
 */

#include "../example.h"

#include <gmsec4/util/TimeUtil.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "json.hpp"
#include "dataset.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace gmsec::api;
using namespace gmsec::api::util;


std::vector<std::string> topics;
std::vector<std::string> jtopics;
std::mutex topic_mutex;
std::condition_variable topic_cv;
std::vector<Message*> messages;
std::mutex message_mutex;

std::string dictionary;
subsystem nav_sys;
std::mutex nav_mutex;
subsystem orb_sys;
std::mutex orb_mutex;
subsystem veh_sys;
std::mutex veh_mutex;
subsystem systemdata;
std::mutex systemdata_mutex;
std::vector<std::time_t> time_vector;
std::mutex sys_mutex;
nlohmann::json j;

class gmsub
{
	public:
		gmsub(Config& config);
		~gmsub();
		bool run();

	private:
		typedef std::vector<std::string> Subjects;

		Config&     config;
		Connection* connection;
		Subjects    subjects;
		SubscriptionInfo** info;
};

gmsub::gmsub(Config &c)
	: config(c),
	  connection(0),
	  subjects(0)
{
	// Initialize config
	example::initialize(c);
}

gmsub::~gmsub()
{
	if (connection)
	{
		for (size_t i = 0; i < subjects.size(); ++i)
		{
			GMSEC_INFO << "Unsubscribing from " << subjects[i].c_str();
			connection->unsubscribe(info[i]);
		}
		delete[] info;
		connection->disconnect();
		Connection::destroy(connection);
	}

	Connection::shutdownAllMiddlewares();
}

bool gmsub::run()
{
	bool success = true;

	//o output GMSEC API version
	GMSEC_INFO << Connection::getAPIVersion();

	try
	{
		//o Create the Connection
		connection = Connection::create(config);

		//o Connect
		connection->connect();

		//o Determine the subjects to listen to
		example::determineSubjects(config, subjects);

		//o Determine runtime settings
		int iterations     = example::get(config, "ITERATIONS", 0);
		int msg_timeout_ms = example::get(config, "MSG_TIMEOUT_MS", GMSEC_WAIT_FOREVER);
		int prog_timeout_s = example::get(config, "PROG_TIMEOUT_S", GMSEC_WAIT_FOREVER);

		if (iterations > 0)
		{
			GMSEC_INFO << "Waiting for up to " << iterations << " messages";
		}

		//o Output the middleware information
		GMSEC_INFO << "Middleware version = " << connection->getLibraryVersion();

		//o Subscribe
		info = new SubscriptionInfo*[subjects.size()];
		for (size_t i = 0; i < subjects.size(); ++i)
		{
			GMSEC_INFO << "Subscribing to " << subjects[i].c_str();
			info[i] = connection->subscribe(subjects[i].c_str());
		}

		//o Wait for, and print out messages
		int    count = 0;
		bool   done = false;
		double prevTime;
		double nextTime;
		double elapsedTime = 0;

		prevTime = TimeUtil::getCurrentTime_s();

		while (!done)
		{
			if (prog_timeout_s != GMSEC_WAIT_FOREVER && elapsedTime >= prog_timeout_s)
			{
				GMSEC_INFO << "Program Timeout Reached!";
				done = true;
				continue;
			}

			Message* message = connection->receive(msg_timeout_ms);

			if (prog_timeout_s != GMSEC_WAIT_FOREVER)
			{
				nextTime = TimeUtil::getCurrentTime_s();
				elapsedTime += (nextTime - prevTime);
				prevTime = nextTime;
			}

			if (!message)
			{
				GMSEC_INFO << "timeout occurred";
			}
			else
			{
				//MessageFieldIterator& it = message->getFieldIterator();
				// while (it.hasNext()) {
				// 	std::cout << it.next().toJSON() << '\n';
				// }
				//GMSEC_INFO << "Received:\n" << message->toXML();
				//std::cout << "Recieved: " << std::string(message->getSubject()) << '\n';
				if(strstr(message->getSubject(), "GMSEC.KSP.MSG.TLM.PROCESSED.N")){
					systemdata_mutex.lock();
					std::cout << "Recieved Navigation>>>>" << '\n';
					MessageFieldIterator& it = message->getFieldIterator();
					while (it.hasNext()) {
						nlohmann::json f = nlohmann::json::parse(it.next().toJSON());
						systemdata.updateMeasurement(f["NAME"],f["VALUE"],message->getUnsignedIntegerValue("PUBLISH-TIME-UL"));
					}
					//std::cout << systemdata.printData() << '\n';
					systemdata_mutex.unlock();
				} else if(strstr(message->getSubject(), "GMSEC.KSP.MSG.TLM.PROCESSED.O")){
					systemdata_mutex.lock();
					std::cout << "Recieved Orbit>>>>" << '\n';
					MessageFieldIterator& it = message->getFieldIterator();
					while (it.hasNext()) {
						nlohmann::json f = nlohmann::json::parse(it.next().toJSON());
						systemdata.updateMeasurement(f["NAME"],f["VALUE"],message->getUnsignedIntegerValue("PUBLISH-TIME-UL"));
					}
					//std::cout << systemdata.printData() << '\n';
					systemdata_mutex.unlock();
				} else if(strstr(message->getSubject(), "GMSEC.KSP.MSG.TLM.PROCESSED.V")){
					systemdata_mutex.lock();
					std::cout << "Recieved Vehicle>>>>" << '\n';
					MessageFieldIterator& it = message->getFieldIterator();
					while (it.hasNext()) {
						nlohmann::json f = nlohmann::json::parse(it.next().toJSON());
						systemdata.updateMeasurement(f["NAME"],f["VALUE"],message->getUnsignedIntegerValue("PUBLISH-TIME-UL"));
					}
					//std::cout << systemdata.printData() << '\n';
					systemdata_mutex.unlock();
				} else {
					std::cout << ">>>>>>Not Parsed." << '\n';
				}
				++count;

				done = (iterations > 0 && count >= iterations);
				done = done || (std::string(message->getSubject()) == "GMSEC.TERMINATE");

				connection->release(message);
			}
		}
	}
	catch (Exception& e)
	{
		GMSEC_ERROR << e.what();
		success = false;
	}

	return success;
}



typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;

class telemetry_server {
	public:
	    typedef websocketpp::connection_hdl connection_hdl;
	    typedef websocketpp::server<websocketpp::config::asio> server;
	    telemetry_server() : m_count(0) {
	        // set up access channels to only log interesting things
	        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	        m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
	        m_endpoint.set_access_channels(websocketpp::log::alevel::app);

	        // Initialize the Asio transport policy
	        m_endpoint.init_asio();

	        // Bind the handlers we are using
	        using websocketpp::lib::placeholders::_1;
					using websocketpp::lib::placeholders::_2;
	        using websocketpp::lib::bind;
	        m_endpoint.set_open_handler(bind(&telemetry_server::on_open,this,_1));
	        m_endpoint.set_close_handler(bind(&telemetry_server::on_close,this,_1));
	        m_endpoint.set_http_handler(bind(&telemetry_server::on_http,this,_1));
					m_endpoint.set_message_handler(bind(&telemetry_server::on_msg,this,::_1,::_2));
	    }

	    void run(std::string docroot, uint16_t port) {
	        std::stringstream ss;
	        ss << "Running telemetry server on port "<< port <<" using docroot=" << docroot;
	        m_endpoint.get_alog().write(websocketpp::log::alevel::app,ss.str());

	        m_docroot = docroot;

	        // listen on specified port
	        m_endpoint.listen(port);

	        // Start the server accept loop
	        m_endpoint.start_accept();

	        // Set the initial timer to start telemetry
	        set_timer();

	        // Start the ASIO io_service run loop
	        try {
	            m_endpoint.run();
	        } catch (websocketpp::exception const & e) {
	            std::cout << e.what() << std::endl;
	        }
	    }

	    void set_timer() {
	        m_timer = m_endpoint.set_timer(
	            1000,
	            websocketpp::lib::bind(
	                &telemetry_server::on_timer,
	                this,
	                websocketpp::lib::placeholders::_1
	            )
	        );
	    }

	    void on_timer(websocketpp::lib::error_code const & ec) {
	        if (ec) {
	            // there was an error, stop telemetry
	            m_endpoint.get_alog().write(websocketpp::log::alevel::app,
	                    "Timer Error: "+ec.message());
	            return;
	        }
					update_msg_queue();
					for (size_t i = 0; i < topics.size(); i++) {
						std::cout << topics.at(i) << '\n';
					}
					for (size_t i = 0; i < jtopics.size(); i++) {
						std::cout << jtopics.at(i) << '\n';
					}
					con_list::iterator it;
					for (it = m_connections.begin(); it != m_connections.end(); ++it) {
						for (size_t i = 0; i < jtopics.size(); i++) {
		        	m_endpoint.send(*it,jtopics.at(i),websocketpp::frame::opcode::text);
		        }
					}
					jtopics.clear();
	        // set timer for next telemetry check
	        set_timer();
	    }

			std::string json_pack(std::string ident, unsigned long time, std::string data){
 				j["type"] = "data";
				j["id"] = ident;
				j["value"]["timestamp"] = time;
				j["value"]["value"] = data;
				return j.dump();
			}

			void update_msg_queue(){
				for(std::vector<std::string>::iterator it = topics.begin();it != topics.end();it++){
					measurement meas =  systemdata.getMeasurement(*it);
					//std::cout << "FOUND:  " << meas.name << '\n';
					jtopics.push_back(json_pack(meas.name,meas.timestamp,meas.v));
				}
			}

			nlohmann::json replyHistory(std::string val){
				//remove whitespace
				val.erase(remove_if(val.begin(),val.end(),isspace),val.end());
				nlohmann::json re;
				return re;
			}



	    void on_http(connection_hdl hdl) {
	        // Upgrade our connection handle to a full connection_ptr
	        server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

	        std::ifstream file;
	        std::string filename = con->get_resource();
	        std::string response;

	        m_endpoint.get_alog().write(websocketpp::log::alevel::app,
	            "http request1: "+filename);

	        if (filename == "/") {
	            filename = m_docroot+"index.html";
	        } else {
	            filename = m_docroot+filename.substr(1);
	        }

	        m_endpoint.get_alog().write(websocketpp::log::alevel::app,
	            "http request2: "+filename);

	        file.open(filename.c_str(), std::ios::in);
	        if (!file) {
	            // 404 error
	            std::stringstream ss;

	            ss << "<!doctype html><html><head>"
	               << "<title>Error 404 (Resource not found)</title><body>"
	               << "<h1>Error 404</h1>"
	               << "<p>The requested URL " << filename << " was not found on this server.</p>"
	               << "</body></head></html>";

	            con->set_body(ss.str());
	            con->set_status(websocketpp::http::status_code::not_found);
	            return;
	        }

	        file.seekg(0, std::ios::end);
	        response.reserve(file.tellg());
	        file.seekg(0, std::ios::beg);

	        response.assign((std::istreambuf_iterator<char>(file)),
	                        std::istreambuf_iterator<char>());

	        con->set_body(response);
	        con->set_status(websocketpp::http::status_code::ok);
	    }

			void on_msg(connection_hdl hdl, server::message_ptr msg) {
			    //std::cout << "Message sent to default handler" << std::endl;
					std::string payload = msg->get_payload();
					std::string cmd = payload.substr(0, payload.find(" ", 0));
					std::string val = payload.substr(cmd.length(), payload.length());
					//echo
					//s->send(hdl, msg->get_payload(), msg->get_opcode());
			    if (cmd == "dictionary") {
			      std::cout << "Returning Dictonary" << std::endl;
						m_endpoint.send(hdl, json_dictionary(dictionary).dump(), websocketpp::frame::opcode::text);
			    } else if(cmd == "subscribe"){
						std::cout << "Subscribing to: " << val << '\n';
						val.erase(remove_if(val.begin(),val.end(),isspace),val.end());
						std::transform(val.begin(),val.end(),val.begin(),::toupper);
						topics.push_back(val);
					} else if(cmd == "unsubscribe"){
						std::cout << "Unsubscribing to: " << val << '\n';
						val.erase(remove_if(val.begin(),val.end(),isspace),val.end());
						topics.erase(std::remove(topics.begin(), topics.end(), val), topics.end());
					} else if(cmd == "history"){
						std::cout << "History of: " << val << '\n';
						m_endpoint.send(hdl, replyHistory(val).dump(), websocketpp::frame::opcode::text);
					} else {
						std::string data = msg->get_payload();
						std::cout << "Data recieved: " << data << std::endl;
					}
			}

	    void on_open(connection_hdl hdl) {
	        m_connections.insert(hdl);
	    }

	    void on_close(connection_hdl hdl) {
	        m_connections.erase(hdl);
	    }
	private:
	    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;

	    server m_endpoint;
	    con_list m_connections;
	    server::timer_ptr m_timer;

	    std::string m_docroot;

	    // Telemetry data
	    uint64_t m_count;
};

int main(int argc, char* argv[])
{
	telemetry_server s;
  std::string docroot;
  uint16_t port = 8081;

	Config config(argc, argv);

	example::addToConfigFromFile(config);

	if (example::isOptionInvalid(config, argc, "gmsub"))
	{
		example::printUsage("gmsub");
		return -1;
	}

	gmsub g = gmsub(config);

	//load dictionary
	std::string filename = example::get(config, "dictionary", "/GMSEC_API/bin/dictionary.json");
	dictionary = load_dictionary(filename);

	//Threads
	std::thread gmsec(&gmsub::run,&g);
	std::this_thread::sleep_for(std::chrono::seconds(5));
	std::thread websocket(&telemetry_server::run,&s,docroot, port);


	int counter= 0;
	while(true){
		std::cout << "Still running..." << counter << '\n';
		counter++;
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

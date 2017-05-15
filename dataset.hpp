#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <mysql/mysql.h>

#include <boost/any.hpp>

#include "../example.h"
#include "json.hpp"

using namespace gmsec::api;
using namespace gmsec::api::util;


std::vector<std::string> telemetry_available;

// struct measurement {
//   std::string name;
// 	std::vector<boost::any> v;
//   std::vector<long> timestamp;
//
//   void addData(boost::any data,long time){
//     v.push_back(data);
//     timestamp.push_back(time);
//   }
// };

struct measurement {
  std::string name;
  std::string v;
  unsigned long timestamp;
};

struct subsystem {
  std::string name;
	std::vector<measurement> v;
  MYSQL *con=mysql_init(NULL);
  MYSQL_RES *res;

  void updateMeasurement(std::string m, std::string value, unsigned long t){
    mysql_real_connect(con,"192.168.1.4","root","root","gmsec",0,NULL,0);
    for(std::vector<measurement>::iterator it = v.begin();it != v.end();++it){
      if(it->name==m){
        it->v = value;
        it->timestamp = t;
        std::string insert = "INSERT INTO " + m + " VALUES(" + value + "," + std::to_string(t) +");";
        mysql_query(con,insert.c_str());
        return;
      }
    }
    measurement meas;
    meas.name = m;
    meas.v = value;
    meas.timestamp = t;
    v.push_back(meas);
    //mysql_query
    std::string str = "CREATE TABLE "+m+"(Timestamp TEXT, Value TEXT);";
    std::cout << str << '\n';
    mysql_query(con,str.c_str());
    std::cout << mysql_error(con) << '\n';
    str = "INSERT INTO " + m + " VALUES(" + value + "," + std::to_string(t) +");";
    mysql_query(con,str.c_str());
    std::cout << mysql_error(con) << '\n';
    return;
  }

  void connectToDB(std::string s,std::string prefix){
    std::cout << "Connecting to server " << s << " version: " << mysql_get_client_info() << '\n';
    mysql_real_connect(con,s.c_str(),"root","root","gmsec",0,NULL,0);
    std::string test = "CREATE TABLE "+ prefix +"(Timestamp TEXT, Value TEXT);";
    mysql_query(con,test.c_str());
    test = "INSERT INTO " + prefix +" VALUES(2000,9999);";
    mysql_query(con,test.c_str());
    mysql_free_result(res);
    mysql_close(con);
    return;
  }

  bool hasMeasurement(std::string m){
    for(std::vector<measurement>::iterator it = v.begin();it != v.end();++it){
      if(it->name==m){
        return true;
      }
    }
    return false;
  }

  measurement getMeasurement(std::string m){
    measurement ret;
    for(std::vector<measurement>::iterator it = v.begin();it != v.end();++it){
      //std::cout << "Searching...." << it->name << m << '\n';
      if(it->name==m){
        ret.name = it->name;
        ret.v = it->v;
        ret.timestamp = it->timestamp;
        std::cout << "GOT MEASUREMENT: " << ret.name << '\n';
        return ret;
      }
    }
    return ret;
  }

  std::string printData(){
    std::string str;
    for(std::vector<measurement>::iterator it = v.begin();it != v.end();++it){
      str += "Data: " + it->name + " " + it->v + " \n";
    }
    return str;
  }
};

std::string load_dictionary(std::string input){
	std::string filename = input;
	std::ifstream file;
	file.open(filename.c_str(), std::ios::in);
	if(!file){
		std::cout << "No File Found" << '\n';
		return "{\"dictionary\":\"null\"}";
	}
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return str;
}

nlohmann::json json_dictionary(std::string dict){
	nlohmann::json d;
	d["type"] = "dictionary";
	d["value"] = nlohmann::json::parse(dict);
	return d;
}

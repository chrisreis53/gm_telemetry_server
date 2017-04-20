#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

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

  void updateMeasurement(std::string m, std::string value, unsigned long t){
    for(std::vector<measurement>::iterator it = v.begin();it != v.end();++it){
      if(it->name==m){
        it->v = value;
        it->timestamp = t;
        return;
      }
    }
    measurement meas;
    meas.name = m;
    meas.v = value;
    meas.timestamp = t;
    v.push_back(meas);
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
      std::cout << "Searching...." << it->name << m << '\n';
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

std::string load_dictionary(){
	std::string filename = "dictionary.json";
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

/**
 * @author Aleksandar Dragojevic aleksndar.dragojevic@epfl.ch
 *
 */

// NOTE: this does not work
// TODO: make it work without stl: implement hashtable and use it here

#ifndef WLPDSTM_CONFIGURATION_H_
#define WLPDSTM_CONFIGURATION_H_

#include <string.h>

namespace wlpdstm {

	struct ConfigItem {
		ConfigItem(std::string n, std::string dv) : name(n), default_value(dv) {
			// empty
		}

		ConfigItem(std::string n, int dv) : name(n), default_value(str_to_int(dv)) {
			// empty
		}
		
		const std::string name;
		std::string value;
		const std::string default_value;

		// TODO leave this for later
		//std::string file_key;
		//std::string cmd_line_long_key;
		//char cmd_line_short_key;
		//std::string description;
	};

	class Config {
		public:
			void Initialize();

			void AddItem(ConfigItem config_item);

			bool GetValueInt(std::string name, int &value);
			bool GetValueString(std::string name, std::string &value);

		private:
			void InitializeDefault();

			static int str_to_int(std::string val);
			static std::string int_to_str(int val);
		private:
			map<std::string, ConfigItem> config_items;
	};
}

void wlpdstm::Config::Initialize() {
	InitializeDefault();
}

void wlpdstm::Config::InitializeDefault() {
	map<std::string, ConfigItem>::itearator iter;

	for(iter = config_items.start();iter != config_items.end();iter++) {
		ConfigItem &config_item = (*iter).second;
		config_item.value = config_item.default_value;
	}
}

void wlpdstm::Config::AddItem(ConfigItem config_item) {
	config_items[config_item.name] = config_item;
}

bool wlpdstm::Config::GetValueString(std::string name, std::string &value) {
	map<std::string, ConfigItem>::iterator iter = config_items.find(name);
	
	if(iter == config_names.end()) {
		return false;
	}
	
	ConfigItem &config_item = (*iter).second;
	value = config_item.value;
	return true;	
}

bool wlpdstm::Config::GetValueInt(std::string name, int &value) {
	str::string string_value;
	bool exists = GetValueString(name, string_value);

	if(!exists) {
		return false;
	}

	value = str_to_int(string_value);
	return true;
}

inline int wlpdstm::Config::str_to_int(std::string str) {
	std::string::const_iterator iter = str.begin();
	std::string buf;
	
	// skip whitespace
	while(iter != str.end()) {
		if(!isspace(*iter)) {
			break;
		}
		
		iter++;
	}
	
	// read digits
	while(iter != str.end()) {
		if(isdigit(*iter)) {
			buf.push_back(*iter);
			iter++;
		} else {
			break;
		}
	}
	
	// check to see if there is some garbage left
	while(iter != str.end()) {
		if(!isspace(*iter)) {
			return -1;
		}
		
		iter++;
	}
	
	istringstream inputStream(buf);
	int ret = -1;
	inputStream >> ret;
	
	return ret;
}

inline std::string wlpdstm::Config::int_to_str(int val) {
	std::stringstream str_buf;
	str_buf << val;
	return str_buf.val();
}


#endif WLPDSTM_CONFIGURATION_H_

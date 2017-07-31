#ifndef __NUMSTRING_H__
#define __NUMSTRING_H__
#include <string>
#include <sstream>

template <typename Type> 
bool StringToNum(const std::string& str, Type &num) {
	std::istringstream iss(str);
	if (!(iss >> num))
		return false;
	return true;
}

template <typename Type> 
bool NumToString(const Type& num, std::string &str) {
	std::ostringstream oss;
	if (!(oss << num))
		return false;
	str = oss.str();
	return true;
}
#endif

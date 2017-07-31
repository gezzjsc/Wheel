#include <string>
#include <sstream>

//demo:	StringToNum<int>(str1, i1)
template <typename Type> 
bool StringToNum(const std::string& str, Type &num) {
	std::istringstream iss(str);
	if (!(iss >> num))
		return false;
	return true;
}

//demo:	NumToString<int>(i1, str4)
template <typename Type> 
bool NumToString(const Type& num, std::string &str) {
	std::ostringstream oss;
	if (!(oss << num))
		return false;
	str = oss.str();
	return true;
}
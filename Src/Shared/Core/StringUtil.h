#pragma once



#define STREAMTOSTRING( x ) dynamic_cast< std::ostringstream & >( \
	(std::ostringstream{} << std::dec << x)).str()

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

std::string &ltrim(std::string &s);
std::string &rtrim(std::string &s);
std::string &trim(std::string &s);

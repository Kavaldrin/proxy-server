#pragma once

#include <string>
#include <vector>
#include <optional>

class ParserHttp {
public:
	ParserHttp(const std::string& p_msg) : msg{p_msg} {};
	
	std::optional<std::string> parse();

	std::optional<std::string> parseStartLine();
	void parseMethod();
	std::string parsePath();
	// void parseQueryString();
	void parseHttpMethod();

	void parseHeaders();
	void parseHeader();

	void parseBody();

private:
	std::vector<std::string> splitString(const std::string& str, const std::string& delims);

	const std::string& msg;
	std::vector<std::string> start_line;
	int end_of_previous_section;
};
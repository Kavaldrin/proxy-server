#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

using HttpRequest_t = std::map<std::string, std::string>;

class ParserHttp {
public:
	ParserHttp(const std::string& p_msg) : msg{p_msg} {};
	
	std::optional<HttpRequest_t> parse();

	std::optional<std::string> parseStartLine();
	void parseMethod();
	std::string parsePath();
	// void parseQueryString();
	void parseHttpMethod();

	void parseHeaders(std::vector<std::string>& headers);
	void parseHeader();

	void parseBody();

private:
	HttpRequest_t http_request;

	const std::string& msg;
	std::vector<std::string> start_line;
	int end_of_previous_section;
};
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <string_view>

using HttpRequest_t = std::map<std::string, std::string>;

class ParserHttp {
public:
	ParserHttp(const std::string_view& p_msg) : msg{p_msg} {};
	
	std::optional<HttpRequest_t> parse();

	std::pair< std::optional<std::string>, std::optional<std::string> >  parseStartLine();
	std::optional<std::string> parseMethod();
	std::string parsePath();
	// void parseQueryString();
	void parseHttpMethod();

	HttpRequest_t parseHeaders(std::vector<std::string>& headers);
	void parseHeader();

	void parseBody();

	bool isHTTPRequest() noexcept;

	std::pair < std::optional<std::string>, std::optional <std::string> > getBaseAddress(std::string baseAddress) noexcept;

private:
	HttpRequest_t http_request;

	const std::string_view& msg;
	std::vector<std::string> start_line;
	int end_of_previous_section;
};
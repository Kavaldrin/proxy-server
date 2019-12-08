#include "parserHttp.h"

#include <algorithm>
#include <iostream>
#include <exception>
#include <list>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>

const std::string HTTP_METHODS[] {
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"TRACE",
	"OPTIONS",
	"CONNECT",
	"PATCH"
};

constexpr const char* END_OF_HEADERS = "\r\n\r\n";
constexpr int NEW_LINE_FROM_PREVIOUS_SECTION = 1;

struct BadMethodException : public std::exception
{ const char* what() const throw() { return "BadMethodException"; } };

struct NoMethodException : public std::exception
{ const char* what() const throw() { return "NoMethodException"; } };

std::optional<HttpRequest_t> ParserHttp::parse() {
	http_request.insert({"MSG", msg.data()});
	auto resp = parseStartLine();
	parseHeader();
	return http_request;
}

std::pair< std::optional<std::string>, std::optional<std::string> > ParserHttp::parseStartLine() {
	end_of_previous_section = msg.find("\n");
	std::string start_line_before_split{msg.begin(), msg.begin()+end_of_previous_section};
	boost::split(start_line, start_line_before_split, boost::is_any_of(" "));

	auto method = parseMethod();
	if(method.has_value())
	{
		return {method, parsePath()};
	}

	return { {} , {} };
}

std::optional<std::string> ParserHttp::parseMethod() {
	std::string header;

	constexpr unsigned NUMBER_OF_ALL_HTTP_METHODS = 9;
	auto nb_of_checked_methods = 0;
	for(const auto& method : HTTP_METHODS) {
		if(start_line[0] == method) {
			header = method;
			break;
		}

		nb_of_checked_methods++;
	}

	http_request.insert({"METHOD", header});

	if (nb_of_checked_methods == NUMBER_OF_ALL_HTTP_METHODS) {
		return {};
	}
	return header;
}

std::string ParserHttp::parsePath() {
	http_request.insert({"PATH", std::string{start_line[1].begin()+1, start_line[1].end()}});
	auto result = std::string{start_line[1].begin(), start_line[1].end()};
	std::remove_if(result.begin(), result.end(), 
	[&](const auto& ch)
		{
			return ch == '\n' || ch == '\r';
		}
	);

	return result;
}

// void ParserHttp::parseQueryString() {}

void ParserHttp::parseHttpMethod() {

}

void ParserHttp::parseHeader() {
	try {
		auto end_of_section = msg.find(END_OF_HEADERS);
		//TO DO if end=_of_section > 8k odrzucamy

		std::string header{msg.begin()+end_of_previous_section + NEW_LINE_FROM_PREVIOUS_SECTION,
						   msg.begin()+end_of_section};
		std::vector<std::string> headers;
		boost::split(headers, header, boost::is_any_of("\n"));

		parseHeaders(headers);
	}
	catch(std::exception& e) {
		std::cerr << e.what();
	}
}

HttpRequest_t ParserHttp::parseHeaders(std::vector<std::string>& headers) {
	HttpRequest_t headers_map;
	std::vector<std::string> headerName_headerVal;

	for(auto header : headers){
		boost::algorithm::split_regex(headerName_headerVal, header, boost::regex(": "));
		headers_map.insert({headerName_headerVal[0], headerName_headerVal[1]});

		std::cout << "h " << headerName_headerVal[0] << ": " 
		                  << headerName_headerVal[1] << std::endl;
	}
	return headers_map;
}

void ParserHttp::parseBody() {

}

bool ParserHttp::isHTTPRequest() noexcept
{
	return (msg.find("HTTP") != std::string::npos);
}

std::optional< std::string > ParserHttp::getBaseAddress(std::string address) noexcept
{

	int startPos = 0;
	for(auto& possibleBegining : {"http://", "https://"})
	{
		if(auto possibleNewPos = address.find(possibleBegining); possibleNewPos != std::string::npos)
		{
			startPos = possibleNewPos + strlen(possibleBegining);
			break;
		}
	}

	for(auto& possibleEnd : {".com", ".pl"})
	{
		auto pos = address.find(possibleEnd);
		if(pos != std::string::npos)
		{
			return std::string(address, startPos, pos + strlen(possibleEnd) - startPos);
		}
	}
	return {};
}
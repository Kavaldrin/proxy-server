#include "parserHttp.h"

#include <algorithm>
#include <iostream>
#include <exception>

#include <boost/algorithm/string.hpp>

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

std::optional<std::string> ParserHttp::parse() {
	auto resp = parseStartLine();
	parseHeader();
	return resp;
}

std::optional<std::string> ParserHttp::parseStartLine() {
	end_of_previous_section = msg.find("\n");
	start_line = splitString(std::string{msg.begin(), msg.begin()+end_of_previous_section}, std::string{" "});

	try {
		parseMethod();
		return parsePath();
	}
	catch(std::exception& e) {
		std::cerr << e.what() << std::endl;
		return std::nullopt;
	}
}

void ParserHttp::parseMethod() {
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

	if (nb_of_checked_methods == NUMBER_OF_ALL_HTTP_METHODS) {
		std::cerr << "no http method\n";
		throw std::exception{};
	}
	if (header != "GET" and header != "CONNECT"){
		std::cerr << "not GET method\n";
		throw std::exception{};
	}
}

std::string ParserHttp::parsePath() {
	return std::string{start_line[1].begin()+1, start_line[1].end()};
}

void ParserHttp::parseHttpMethod() {

}

void ParserHttp::parseHeader() {
	try {
		auto end_of_section = msg.find(END_OF_HEADERS);


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

void ParserHttp::parseHeaders(std::vector<std::string>& headers) {
	for(auto header : headers)
		std::cout << header << std::endl;
}

void ParserHttp::parseBody() {

}

std::vector<std::string> ParserHttp::splitString(const std::string& str, const std::string& delims) {
	std::vector<std::string> cont;
	std::size_t current, previous = 0;
    
    current = str.find_first_of(delims);
    while (current != std::string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find_first_of(delims, previous);
    }

    cont.push_back(str.substr(previous, current - previous));

    return cont;
}

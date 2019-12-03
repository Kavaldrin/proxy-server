#include "parserHttp.h"

#include <algorithm>
#include <iostream>
#include <exception>

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
		std::cout << "throw exception: " << e.what() << std::endl;
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
		std::cout << "no http method\n";
		throw std::exception{};
	}
	if (header != "GET")
		throw std::exception{};
}

std::string ParserHttp::parsePath() {
	return std::string{start_line[1].begin()+1, start_line[1].end()};
}

// void ParserHttp::parseQueryString() {}

void ParserHttp::parseHttpMethod() {

}

void ParserHttp::parseHeader() {
	auto end_of_section = msg.find("\n\n");

	// auto headers = splitString(std::string{msg.begin()+end_of_previous_section, msg.begin()+end_of_section}, std::string{"\n"});


	if(end_of_section != std::string::npos)
		std::cout << "mlekeokeokek\n" << std::string{msg.begin()+end_of_previous_section, msg.begin()+end_of_section};
	else
		std::cout << "kjdfbasb\n";

	// for(auto i : headers) {
	// 	std::cout << i;
	// }
	// std::cout << "\n\n";
}

void ParserHttp::parseHeaders() {

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

#pragma once

#include <optional>
#include <string>
#include <vector>

class HttpResponseBuilder {
public:
	HttpResponseBuilder(const std::optional<std::string>& response) : resp{response} {}
	std::string build();
	std::string makeContentType();
	std::vector<char> readFile (const char* path);

private:
	const std::optional<std::string>& resp;
};
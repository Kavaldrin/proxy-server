#include "httpResponseBuilder.h"

#include <iostream>
#include <fstream>

std::string HttpResponseBuilder::build() {
	std::string http_response;

	if(not resp) {
		http_response += "HTTP/1.1 404 NOT FOUND\n\n";
		return http_response;
	}

	try {
		auto result = readFile(resp->c_str());
		std::string file{result.begin(), result.end()};;

		http_response += "HTTP/1.1 200 OK\n";
		http_response += makeContentType();
		http_response += "\n";
		http_response += file;

		return http_response;
	}
	catch(std::exception& e) {
		std::cout << "throw exception: " << e.what() << std::endl;
		http_response += "HTTP/1.1 404 NOT FOUND\n\n";
		return http_response;
	}
}

std::string HttpResponseBuilder::makeContentType() {
	std::string content_type{"Content-Type: "};

	auto dot = resp->rfind(".");
	if(dot == std::string::npos) {
		content_type += "application/octet-stream\n";
		return content_type;
	}

	dot += 1;
	std::string file_type{resp->begin()+dot, resp->end()};

	if(file_type == "html")
		content_type += "text/html; charset=UTF-8\n";
	else if(file_type == "jpeg" or file_type == "jpg")
		content_type += "image/jpeg\n";
	else if(file_type == "js")
		content_type += "application/javascript\n";
	else
		content_type += "application/octet-stream\n";

	return content_type;
}

std::vector<char> HttpResponseBuilder::readFile (const char* path)
{
  using namespace std;

  vector<char> result;
  ifstream file (path, ios::in|ios::binary|ios::ate);
  file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  result.resize(file.tellg(), 0);
  file.seekg (0, ios::beg);
  file.read (result.data(), result.size());
  file.close();

  return result;
}

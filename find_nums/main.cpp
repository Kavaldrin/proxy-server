#include "NumFinder.h"

#include <string>

#include <iostream>
#include <string>
#include <regex>

int main() {
	std::string in{"12341234123412341234123426**00000000 000000  0000 00000072**1111 1111 1111 1111 1111 1111 60*1111 1111 1111 1111 1111 1111 70l00000000 000000 0000 00000072"};
	std::string replace{"21372137213721372137213726"};
	NumFinder finder(in, replace);

	std::cout << in << std::endl;
	std::cout << finder.findNum() << std::endl;
}
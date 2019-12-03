#include "NumFinder.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace mp = boost::multiprecision;

std::string NumFinder::findNum() {
    std::string output{input};
    std::smatch base_match;

    int preprefix = 0;

    while (std::regex_search(input, base_match, re)) {
    	if(isNumWeLookFor(base_match[2])) {
    		auto addToPrefix = base_match[1].length() == 1 ? 1 : 0;
    		auto addToSuffix = base_match[5].length() == 1 ? 1 : 0;

    		auto sumPrefix = base_match.prefix().length() + addToPrefix + preprefix;
    		auto sumSuffix = base_match.suffix().length() + addToSuffix;

    		changeNums(output.begin() + sumPrefix, output.end() - sumSuffix);
    	}

    	preprefix += base_match[0].length() + base_match.prefix().length();
        input = base_match.suffix();
    }

    return output;
}

bool NumFinder::isNumWeLookFor(std::string num) {
	boost::erase_all(num, " ");
	mp::int128_t firstTwo{mp::cpp_int{std::string{num.begin(), num.begin()+2}}};
	num.erase(0, 2);
	num.erase(0, std::min(num.find_first_not_of('0'), num.size()-1));
	mp::int128_t numAsInt{num};
	numAsInt *= 1000000;
	numAsInt += 252100;
	numAsInt += firstTwo;
	numAsInt %= 97;

	if(numAsInt == 1) return true;
	return false;
}

void NumFinder::changeNums(std::string::iterator beg, std::string::iterator end) {
	auto iter = definedNum.begin();
	for(auto i = beg; i != end; ++i) {
		if(std::isdigit(*i))
			*i = *iter++;
	}
}
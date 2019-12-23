#pragma once

#include <string>
#include <regex>

class NumFinder {
public:
	NumFinder(const std::string& p_input, const std::string& p_definedNum)
	 : input(p_input), definedNum(p_definedNum) {}
	std::string findNum();

private:
	bool isNumWeLookFor(std::string numRege);
	void changeNums(std::string::iterator beg, std::string::iterator end);

	const std::regex re{"(\\D|^)((\\d( |)){26})(\\D|$)"};
	std::string input;
	std::string definedNum;
};
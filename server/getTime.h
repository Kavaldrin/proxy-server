#pragma once

#include<time.h>
#include<sstream>

std::string getTime (const char* format)
{
  time_t rawtime;
  struct tm* timeinfo;
  char output[40];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  std::stringstream formatString;
  formatString<<"server time: "<<format;
  strftime (output, sizeof(output), formatString.str().data(), timeinfo);

  return output;
}

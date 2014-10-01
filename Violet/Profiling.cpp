#include "Profiling.hpp"

Profile::duration Profile::comp;
std::map<const char*, Profile::duration> Profile::data;
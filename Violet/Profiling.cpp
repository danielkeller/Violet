#include "Profiling.hpp"

Profile::duration Profile::comp;
std::map<const char*, std::pair<int, Profile::duration>> Profile::data;
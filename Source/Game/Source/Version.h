#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#ifndef STRING
#	define _STRING(str) #str
#	define STRING(str) _STRING(str)
#endif

#if VERSION_MINOR == 0
#	define VERSION_STR STRING(VERSION_MAJOR)
#else
#	define VERSION_STR STRING(VERSION_MAJOR) "." STRING(VERSION_MINOR)
#endif

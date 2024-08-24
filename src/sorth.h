
#pragma once



#if   defined (__WIN32)  \
   || defined (_WIN32)   \
   || defined (WIN32)    \
   || defined (_WIN64)   \
   || defined (__WIN64)  \
   || defined (WIN64)    \
   || defined (__WINNT)

    #define IS_WINDOWS 1

#endif



#if    defined(unix)      \
    || defined(__unix__)  \
    || defined(__unix)    \
    || defined(__APPLE__)

    #define IS_UNIX 1

#endif



#ifdef IS_WINDOWS

	#define _CRT_SECURE_NO_WARNINGS 1

    #include <Windows.h>

#endif


#include <iostream>
#include <filesystem>
#include <list>
#include <vector>
#include <memory>
#include <string>
#include <variant>
#include <optional>
#include <fstream>
#include <sstream>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <map>
#include <stack>
#include <cstring>
#include <cassert>



#include "location.h"
#include "error.h"
#include "source_buffer.h"
#include "tokenize.h"
#include "contextual_list.h"
#include "value.h"
#include "dictionary.h"
#include "operation_code.h"
#include "array.h"
#include "byte_buffer.h"
#include "data_object.h"
#include "hash_table.h"
#include "code_constructor.h"
#include "interpreter.h"
#include "builtin_words.h"
#include "terminal_words.h"

#if defined(IS_UNIX)

	#include "posix_io_words.h"

#elif defined(IS_WINDOWS)

    #include "win_io_words.h"

#endif

#include "user_words.h"

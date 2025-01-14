
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

    #if defined(__APPLE__)

        #define IS_MACOS 1

    #endif

#endif



#ifdef IS_WINDOWS

	#define _CRT_SECURE_NO_WARNINGS 1

    #include <Windows.h>

    #undef min
    #undef max

#endif



#ifdef SORTH_EXPORT

    #if (IS_WINDOWS == 1)

        #define SORTH_API __declspec(dllexport)

    #else

        #define SORTH_API __attribute__((visibility("default")))

    #endif

#else

    #if (IS_WINDOWS == 1)

        #define SORTH_API __declspec(dllimport)

    #else

        #define SORTH_API

    #endif

#endif



#include <stddef.h>
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
#include <condition_variable>
#include <mutex>
#include <thread>



#include "location.h"
#include "error.h"
#include "source-buffer.h"
#include "tokenize.h"
#include "contextual-list.h"
#include "value.h"
#include "word-function.h"
#include "dictionary.h"
#include "instruction.h"
#include "array.h"
#include "byte-buffer.h"
#include "data-object.h"
#include "hash-table.h"
#include "compile-context.h"
#include "blocking-value-queue.h"
#include "interpreter.h"
#include "core-words.h"
#include "terminal-words.h"
#include "ffi-words.h"
#include "user-words.h"
#include "jit.h"


#if (IS_UNIX == 1)

	#include "io-words-posix.h"

#elif (IS_WINDOWS == 1)

    #include "io-words-windows.h"

#endif

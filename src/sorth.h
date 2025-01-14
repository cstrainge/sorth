
#pragma once



#if    defined (__WIN32)  \
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



#if (IS_WINDOWS == 1)

	#define _CRT_SECURE_NO_WARNINGS 1

    #include <Windows.h>

    #undef min
    #undef max

#endif



#if (SORTH_EXPORT == 1)

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



#include "lang/source/location.h"
#include "error.h"
#include "lang/source/source-buffer.h"
#include "lang/source/tokenize.h"
#include "run-time/data-structures/contextual-list.h"
#include "run-time/data-structures/value.h"
#include "run-time/data-structures/word-function.h"
#include "run-time/data-structures/dictionary.h"
#include "lang/code/instruction.h"
#include "run-time/data-structures/array.h"
#include "run-time/data-structures/byte-buffer.h"
#include "run-time/data-structures/data-object.h"
#include "run-time/data-structures/hash-table.h"
#include "lang/code/compile-context.h"
#include "run-time/data-structures/blocking-value-queue.h"
#include "run-time/interpreter/interpreter.h"
#include "run-time/built-ins/core-words/core-words.h"
#include "run-time/built-ins/terminal-words.h"
#include "run-time/built-ins/ffi-words.h"
#include "run-time/built-ins/user-words.h"
#include "lang/code/jit.h"


#if (IS_UNIX == 1)

	#include "run-time/built-ins/io-words-posix.h"

#elif (IS_WINDOWS == 1)

    #include "run-time/built-ins/io-words-windows.h"

#endif

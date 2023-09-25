
#pragma once


#ifdef _WIN64

	#define _CRT_SECURE_NO_WARNINGS 1

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

#if defined(__APPLE__) || defined(__linux__)

	#include "posix_io_words.h"

#endif

#include "user_words.h"

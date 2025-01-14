
#if   defined __WIN32  \
   || defined _WIN32   \
   || defined WIN32    \
   || defined _WIN64   \
   || defined __WIN64  \
   || defined WIN64    \
   || defined __WINNT

#pragma once


namespace sorth
{


    SORTH_API void register_io_words(InterpreterPtr& interpreter);


}

#endif // _WIN32

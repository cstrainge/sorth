#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#pragma once


namespace sorth
{


    void register_io_words(InterpreterPtr& interpreter);


}

#endif // __unix__

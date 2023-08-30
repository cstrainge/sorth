
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        // This part requires posix.  If we want to work on another os, this needs to be ported.
        // We're using this to enable and disable the terminal emulator's raw mode.
        struct termios orig_termios;
        bool is_in_raw_mode = false;


        void word_term_raw_mode(InterpreterPtr& interpreter)
        {
            auto requested_on = as_numeric<bool>(interpreter, interpreter->pop());

            if (requested_on && (!is_in_raw_mode))
            {
                struct termios raw = orig_termios;

                is_in_raw_mode = true;

                tcgetattr(STDIN_FILENO, &orig_termios);
                raw.c_lflag &= ~(ECHO | ICANON);
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            }
            else if ((!requested_on) && is_in_raw_mode)
            {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            }
        }


        void word_term_flush(InterpreterPtr& interpreter)
        {
            std::cout << std::flush;
        }


        void word_term_key(InterpreterPtr& interpreter)
        {
            char next[2] = { 0 };
            int read_chars = 0;

            do
            {
                read_chars = read(STDIN_FILENO, next, 1);
            }
            while (read_chars == 0);

            interpreter->push(std::string(next));
        }


        void word_term_read_line(InterpreterPtr& interpreter)
        {
            std::string line;
            std::getline(std::cin, line);

            interpreter->push(line);
        }


        void word_term_write(InterpreterPtr& interpreter)
        {
            std::cout << interpreter->pop();
        }


    }


    void register_terminal_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "term.raw_mode", word_term_raw_mode);
        ADD_NATIVE_WORD(interpreter, "term.flush", word_term_flush);

        ADD_NATIVE_WORD(interpreter, "term.key", word_term_key);
        ADD_NATIVE_WORD(interpreter, "term.readline", word_term_read_line);
        ADD_NATIVE_WORD(interpreter, "term.!", word_term_write);
    }


}

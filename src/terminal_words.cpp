
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        // This part requires posix.  If we want to work on another os, this needs to be ported.
        // We're using this to enable and disable the terminal emulator's raw mode.
        struct termios original_termios;
        bool is_in_raw_mode = false;


        void word_term_raw_mode(InterpreterPtr& interpreter)
        {
            auto requested_on = as_numeric<bool>(interpreter, interpreter->pop());

            if (requested_on && (!is_in_raw_mode))
            {
                struct termios raw = original_termios;

                auto result = tcgetattr(STDIN_FILENO, &original_termios);

                throw_error_if(result == -1, *interpreter,
                               "Could not read terminal mode information, " +
                               std::string(strerror(errno)) + ".");

                raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
                raw.c_oflag &= ~(OPOST);
                raw.c_cflag |= (CS8);
                raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
                raw.c_cc[VMIN] = 1;

                result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

                throw_error_if(result == -1, *interpreter,
                               "Could not set terminal mode, " +
                               std::string(strerror(errno)) + ".");

                is_in_raw_mode = true;
            }
            else if ((!requested_on) && is_in_raw_mode)
            {
                auto result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);

                throw_error_if(result == -1, *interpreter,
                               "Could not reset terminal mode, " +
                               std::string(strerror(errno)) + ".");

                is_in_raw_mode = false;
            }
        }


        void word_term_flush(InterpreterPtr& interpreter)
        {
            std::cout << std::flush;
        }


        void word_term_size(InterpreterPtr& interpreter)
        {
            struct winsize size;

            auto result = ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

            throw_error_if(result == -1, *interpreter,
                               "Could not read terminal information, " +
                               std::string(strerror(errno)) + ".");

            interpreter->push((int64_t)size.ws_row);
            interpreter->push((int64_t)size.ws_col);
        }


        void word_term_key(InterpreterPtr& interpreter)
        {
            char next[2] = { 0 };
            ssize_t read_chars = 0;

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


        void word_term_is_printable(InterpreterPtr& interpreter)
        {
            auto str = as_string(interpreter, interpreter->pop());

            throw_error_if(str.size() != 1, *interpreter,
                           "Expected single character.");

            bool result =    (str[0] >= 32)
                          || (str[0] == '\n')
                          || (str[0] == '\t');

            interpreter->push(result);
        }


    }


    void register_terminal_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "term.raw_mode", word_term_raw_mode,
                        "Enter or leave the terminal's 'raw' mode.",
                        "bool -- ");


        ADD_NATIVE_WORD(interpreter, "term.flush", word_term_flush,
                        "Flush the terminals buffers.",
                        " -- ");

        ADD_NATIVE_WORD(interpreter, "term.size@", word_term_size,
                        "Return the number or characters in the rows and columns.",
                        " -- columns rows");


        ADD_NATIVE_WORD(interpreter, "term.key", word_term_key,
                        "Read a keypress from the terminal.",
                        " -- character");

        ADD_NATIVE_WORD(interpreter, "term.readline", word_term_read_line,
                        "Read a line of text from the terminal.",
                        " -- string");

        ADD_NATIVE_WORD(interpreter, "term.!", word_term_write,
                        "Write a value to the terminal.",
                        "value -- ");


        ADD_NATIVE_WORD(interpreter, "term.is_printable?", word_term_is_printable,
                        "Is the given character printable?",
                        "character -- bool");
    }


}

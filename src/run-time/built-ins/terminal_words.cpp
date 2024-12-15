
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#if defined(__APPLE__) || defined(__linux__)

    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>

#endif

#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {

        #if defined(__APPLE__) || defined(__linux__)


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
                } while (read_chars == 0);

                interpreter->push(std::string(next));
            }


        #elif defined(_WIN64)


            void init_win_console()
            {
                SetConsoleCP(CP_UTF8);
                SetConsoleOutputCP(CP_UTF8);
            }


            void word_term_raw_mode(InterpreterPtr& interpreter)
            {
                auto flush_events =
                    []()
                    {
                        HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);
                        DWORD dwRead;
                        INPUT_RECORD ir;
                        DWORD numberOfEvents = 0;
                        PeekConsoleInput(std_in_handle, nullptr, 0, &numberOfEvents);

                        if (numberOfEvents > 0)
                        {
                            while (true)
                            {
                                ReadConsoleInput(std_in_handle, &ir, 1, &dwRead);

                                if (   ir.EventType == KEY_EVENT
                                    && ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
                                {
                                    break;
                                }
                            }
                        }
                    };

                static DWORD input_mode;
                static DWORD output_mode;
                static bool is_in_raw_mode = false;

                HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);

                auto requested_on = as_numeric<bool>(interpreter, interpreter->pop());

                BOOL result = FALSE;

                if (requested_on && (!is_in_raw_mode))
                {
                    result = GetConsoleMode(std_in_handle, &input_mode);
                    throw_windows_error_if(!result, *interpreter, "Get console input mode: ",
                                            GetLastError());

                    result = GetConsoleMode(std_out_handle, &output_mode);
                    throw_windows_error_if(!result, *interpreter, "Get console input mode: ",
                                            GetLastError());


                    DWORD new_input_mode = input_mode;
                    DWORD new_output_mode = output_mode;

                    new_input_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT |
                                        ENABLE_PROCESSED_INPUT);
                    new_input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

                    new_output_mode &= ~( ENABLE_INSERT_MODE | DISABLE_NEWLINE_AUTO_RETURN );
                    new_output_mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;


                    result = SetConsoleMode(std_in_handle, new_input_mode);
                    throw_windows_error_if(!result, *interpreter, "Set console input mode: ",
                                            GetLastError());

                    result = SetConsoleMode(std_out_handle, new_output_mode);
                    throw_windows_error_if(!result, *interpreter, "Set console output mode: ",
                                            GetLastError());

                    flush_events();
                    is_in_raw_mode = true;
                }
                else if ((!requested_on) && is_in_raw_mode)
                {
                    result = SetConsoleMode(std_in_handle, input_mode);
                    throw_windows_error_if(!result, *interpreter, "Set console input mode: ",
                                            GetLastError());

                    result = SetConsoleMode(std_out_handle, output_mode);
                    throw_windows_error_if(!result, *interpreter, "Set console output mode: ",
                                            GetLastError());

                    flush_events();
                    is_in_raw_mode = false;
                }
            }




            void word_term_size(InterpreterPtr& interpreter)
            {
                HANDLE std_out_handle = INVALID_HANDLE_VALUE;
                CONSOLE_SCREEN_BUFFER_INFO info = { 0 };

                std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

                auto result = GetConsoleScreenBufferInfo(std_out_handle, &info);

                if (!result)
                {
                    throw_windows_error(*interpreter, "Could not get console information: ",
                                        GetLastError());
                }

                interpreter->push((int64_t)info.dwMaximumWindowSize.X);
                interpreter->push((int64_t)info.dwMaximumWindowSize.Y);
            }




            void word_term_key(InterpreterPtr& interpreter)
            {
                HANDLE std_in_handle = GetStdHandle(STD_INPUT_HANDLE);
                char buffer = 0;
                DWORD num_read = 0;

                auto result = ReadConsoleA(std_in_handle, &buffer, 1, &num_read, nullptr);

                throw_windows_error_if(!result, *interpreter, "Read console error: ",
                                       GetLastError());

                interpreter->push(std::string(1, buffer));
            }


        #endif


        void word_term_flush(InterpreterPtr& interpreter)
        {
            std::cout << std::flush;
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
        #if defined(_WIN64)

            init_win_console();

        #endif


        ADD_NATIVE_WORD(interpreter, "term.raw_mode", word_term_raw_mode,
                        "Enter or leave the terminal's 'raw' mode.",
                        "bool -- ");

        ADD_NATIVE_WORD(interpreter, "term.size@", word_term_size,
                        "Return the number or characters in the rows and columns.",
                        " -- columns rows");

        ADD_NATIVE_WORD(interpreter, "term.key", word_term_key,
                        "Read a keypress from the terminal.",
                        " -- character");

        ADD_NATIVE_WORD(interpreter, "term.flush", word_term_flush,
                        "Flush the terminals buffers.",
                        " -- ");

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

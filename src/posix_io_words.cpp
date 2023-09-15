
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "sorth.h"



namespace sorth
{


    using namespace internal;


    namespace
    {


        bool call_exists(const char* file_path)
        {
            struct stat stat_buffer;

            auto result = stat(file_path, &stat_buffer);
            return result == 0;
        }


        void call_open(InterpreterPtr& interpreter, int flags)
        {
            auto file_mode = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto file_path = as_string(interpreter, interpreter->pop());

            bool is_new_file = false;

            int fd;

            if (!call_exists(file_path.c_str()))
            {
                is_new_file = true;
            }

            do
            {
                errno = 0;
                fd = open(file_path.c_str(), file_mode | flags, 0);
            }
            while ((fd == -1) && (errno == EINTR));

            throw_error_if(fd == -1,
                           *interpreter,
                           "File could not be opened: " +
                           std::string(strerror(errno)) + ".");

            if (is_new_file)
            {
                auto result = fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                throw_error_if(result != 0,
                            *interpreter,
                            "File permissions could not be set: " +
                            std::string(strerror(errno)) + ".");
            }

            interpreter->push(fd);
        }


        off_t call_file_size(InterpreterPtr& interpreter, int fd)
        {
            struct stat buffer;

            if (fstat(fd, &buffer) == -1)
            {
                std::string message;

                switch (errno)
                {
                    case EBADF:     message = "Bad file descriptor."; break;
                    case EIO:       message = "I/O error occurred."; break;
                    case EOVERFLOW: message = "File information exceeds buffer limitations."; break;

                    default:        message = "Unexpected I/O error occurred."; break;
                }

                throw_error(*interpreter, message);
            }

            return buffer.st_size;
        }


        void call_write(InterpreterPtr interpreter, int fd, const void* buffer, size_t total_size)
        {
            ssize_t result = 0;

            do
            {
                errno = 0;
                result = write(fd, buffer, total_size);

                if (result > 0)
                {
                    total_size = total_size - result;
                }
                else if ((result == -1) && (errno != EINTR))
                {
                    break;
                }
            }
            while (result < total_size);

            throw_error_if(result == -1,
                           *interpreter,
                           "FD could not be written to " + std::string(strerror(errno)) + ".");
        }


        void word_file_open(InterpreterPtr& interpreter)
        {
            call_open(interpreter, O_CREAT);
        }


        void word_file_create(InterpreterPtr& interpreter)
        {
            call_open(interpreter, O_TRUNC | O_CREAT);
        }


        void word_file_close(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            int result;

            errno = 0;

            do
            {
                result = close(fd);
            }
            while ((result == -1) && (errno == EINTR));

            throw_error_if(result == -1,
                           *interpreter,
                           "FD could not closed, " + std::string(strerror(errno)) + ".");
        }


        void word_socket_connect(InterpreterPtr& interpreter)
        {
            auto path = as_string(interpreter, interpreter->pop());

            struct sockaddr_un address;

            auto fd = socket(AF_UNIX, SOCK_STREAM, 0);

            throw_error_if(fd == -1, *interpreter,
                           "Could not open socket fd, " + std::string(strerror(errno)) + ".");

            memset(&address, 0, sizeof(struct sockaddr_un));

            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

            auto result = connect(fd, (struct sockaddr*)&address, sizeof(address));

            throw_error_if(result == -1, *interpreter,
                           "Could connect to socket, " + std::string(strerror(errno)) + ".");

            interpreter->push(fd);
        }


        void word_file_size_read(InterpreterPtr& interpreter)
        {
            struct stat buffer;
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());

            errno = 0;

            if (fstat(fd, &buffer) == -1)
            {
                std::string message;

                switch (errno)
                {
                    case EBADF:     message = "Bad file descriptor."; break;
                    case EIO:       message = "I/O error occurred."; break;
                    case EOVERFLOW: message = "File information exceeds buffer limitations."; break;

                    default:        message = "Unexpected I/O error occurred."; break;
                }

                throw_error(*interpreter, message);
            }

            interpreter->push((int64_t)buffer.st_size);
        }


        void word_file_exists(InterpreterPtr& interpreter)
        {
            auto file_path = as_string(interpreter, interpreter->pop());

            interpreter->push(call_exists(file_path.c_str()));
        }


        void word_file_is_open(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            interpreter->push(fcntl(fd, F_GETFD) != -1);
        }


        void word_file_is_eof(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());

            auto position = lseek(fd, 0, SEEK_CUR);
            auto size = call_file_size(interpreter, fd);

            interpreter->push(position >= size);
        }


        void word_file_read(InterpreterPtr& interpreter)
        {
            throw_error(*interpreter, "This word is currently unimplemented.");
        }


        void word_file_read_character(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());

            ssize_t result = 0;
            char next;

            do
            {
                errno = 0;
                result = read(fd, &next, 1);
            }
            while ((result == -1) && (errno == EINTR));

            throw_error_if(result == -1,
                           *interpreter,
                           "FD could not be read from " + std::string(strerror(errno)) + ".");

            std::string new_string(1, next);
            interpreter->push(new_string);
        }


        void word_file_read_string(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            size_t size = as_numeric<int64_t>(interpreter, interpreter->pop());

            char buffer[size + 1];
            memset(buffer, 0, size + 1);

            ssize_t result = 0;
            size_t read_bytes = 0;

            do
            {
                errno = 0;

                do
                {
                    result = read(fd, &buffer[read_bytes], size - read_bytes);

                    if (result > 0)
                    {
                        read_bytes += result;
                    }
                }
                while ((result > 0) && (read_bytes < size));
            }
            while ((result == -1) && (errno == EINTR));

            throw_error_if(result == -1,
                           *interpreter,
                           "FD could not be read from " + std::string(strerror(errno)) + ".");

            std::string new_string = buffer;
            interpreter->push(new_string);
        }


        void word_file_write(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto value = interpreter->pop();

            if (std::holds_alternative<ByteBufferPtr>(value))
            {
                auto buffer = as_byte_buffer(interpreter, value);

                call_write(interpreter, fd, buffer->data_ptr(), buffer->size());
            }
            else
            {
                std::stringstream buffer;

                buffer << value;

                call_write(interpreter, fd, buffer.str().c_str(), buffer.str().size());
            }
        }


        void word_file_line_read(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            std::string line;
            char next = 0;
            ssize_t result = 0;

            do
            {
                errno = 0;

                do
                {
                    result = read(fd, &next, 1);

                    if ((result == 1) && (next != '\n'))
                    {
                        line += next;
                    }
                }
                while ((next != '\n') && (result == 1));
            }
            while ((result == -1) && (errno == EINTR));

            throw_error_if(result == -1,
                           *interpreter,
                           "FD could not be read from " + std::string(strerror(errno)) + ".");

            interpreter->push(line);
        }


        void word_file_line_write(InterpreterPtr& interpreter)
        {
            int fd = (int)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto line = as_string(interpreter, interpreter->pop());

            bool has_nl = line[line.size() - 1] == '\n';
            size_t total_size = has_nl
                              ? line.size()
                              : line.size() + 1;

            char buffer[total_size];
            memcpy(buffer, line.c_str(), line.size());

            if (!has_nl)
            {
                buffer[total_size - 1] = '\n';
            }

            call_write(interpreter, fd, buffer, total_size);
        }

    }


    void register_io_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "file.open", word_file_open,
                        "Open an existing file and return a fd.",
                        "path flags -- fd");

        ADD_NATIVE_WORD(interpreter, "file.create", word_file_create,
                        "Create/open a file and return a fd.",
                        "path flags -- fd");

        ADD_NATIVE_WORD(interpreter, "file.close", word_file_close,
                        "Take a fd and close it.",
                        "fd -- ");


        ADD_NATIVE_WORD(interpreter, "socket.connect", word_socket_connect,
                        "Connect to Unix domain socket at the given path.",
                        "path -- fd");


        ADD_NATIVE_WORD(interpreter, "file.size@", word_file_size_read,
                        "Return the size of a file represented by a fd.",
                        "fd -- size");


        ADD_NATIVE_WORD(interpreter, "file.exists?", word_file_exists,
                        "Does the file at the given path exist?",
                        "path -- bool");

        ADD_NATIVE_WORD(interpreter, "file.is_open?", word_file_is_open,
                        "Is the fd currently valid?",
                        "fd -- bool");

        ADD_NATIVE_WORD(interpreter, "file.is_eof?", word_file_is_eof,
                        "Is the file pointer at the end of the file?",
                        "fd -- bool");


        ADD_NATIVE_WORD(interpreter, "file.@", word_file_read,
                        "Read from a given file.  (Unimplemented.)",
                        " -- ");

        ADD_NATIVE_WORD(interpreter, "file.char@", word_file_read_character,
                        "Read a character from a given file.",
                        "fd -- character");

        ADD_NATIVE_WORD(interpreter, "file.string@", word_file_read_string,
                        "Read a a string of a specified length from a given file.",
                        "size fd -- string");

        ADD_NATIVE_WORD(interpreter, "file.!", word_file_write,
                        "Write a value as text to a file, unless it's a ByteBuffer.",
                        "value fd -- ");


        ADD_NATIVE_WORD(interpreter, "file.line@", word_file_line_read,
                        "Read a full line from a file.",
                        "fd -- string");

        ADD_NATIVE_WORD(interpreter, "file.line!", word_file_line_write,
                        "Write a string as a line to the file.",
                        "string fd -- ");


        ADD_NATIVE_WORD(interpreter, "file.r/o", [](auto intr) { intr->push(O_RDONLY); },
                        "Constant for opening a file as read only.",
                        " -- flag");

        ADD_NATIVE_WORD(interpreter, "file.w/o", [](auto intr) { intr->push(O_WRONLY); },
                        "Constant for opening a file as write only.",
                        " -- flag");

        ADD_NATIVE_WORD(interpreter, "file.r/w", [](auto intr) { intr->push(O_RDWR); },
                        "Constant for opening a file for both reading and writing.",
                        " -- flag");

    }


}

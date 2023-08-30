
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
            auto file_mode = as_numeric<int64_t>(interpreter, interpreter->pop());
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
                           interpreter->get_current_location(),
                           "File could not be opened: " +
                           std::string(strerror(errno)) + ".");

            if (is_new_file)
            {
std::cout << "setting file permissions." << std::endl;
                auto result = fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                throw_error_if(result != 0,
                            interpreter->get_current_location(),
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

                throw_error(interpreter->get_current_location(), message);
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
                           interpreter->get_current_location(),
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
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());
            int result;

            do
            {
                result = close(fd);
            }
            while (result == EINTR);
        }


        void word_file_size_read(InterpreterPtr& interpreter)
        {
            struct stat buffer;
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());

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

                throw_error(interpreter->get_current_location(), message);
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
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());
            interpreter->push(fcntl(fd, F_GETFD) != -1);
        }


        void word_file_is_eof(InterpreterPtr& interpreter)
        {
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());

            auto position = lseek(fd, 0, SEEK_CUR);
            auto size = call_file_size(interpreter, fd);

            interpreter->push(position >= size);
        }


        void word_file_read(InterpreterPtr& interpreter)
        {
        }


        void word_file_write(InterpreterPtr& interpreter)
        {
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());
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
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());
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
                           interpreter->get_current_location(),
                           "FD could not be read from " + std::string(strerror(errno)) + ".");

            interpreter->push(line);
        }


        void word_file_line_write(InterpreterPtr& interpreter)
        {
            int fd = as_numeric<int64_t>(interpreter, interpreter->pop());
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
        ADD_NATIVE_WORD(interpreter, "file.open", word_file_open);
        ADD_NATIVE_WORD(interpreter, "file.create", word_file_create);
        ADD_NATIVE_WORD(interpreter, "file.close", word_file_close);

        ADD_NATIVE_WORD(interpreter, "file.size@", word_file_size_read);

        ADD_NATIVE_WORD(interpreter, "file.exists?", word_file_exists);
        ADD_NATIVE_WORD(interpreter, "file.is_open?", word_file_is_open);
        ADD_NATIVE_WORD(interpreter, "file.is_eof?", word_file_is_eof);

        ADD_NATIVE_WORD(interpreter, "file.@", word_file_read);
        ADD_NATIVE_WORD(interpreter, "file.!", word_file_write);

        ADD_NATIVE_WORD(interpreter, "file.line@", word_file_line_read);
        ADD_NATIVE_WORD(interpreter, "file.line!", word_file_line_write);

        ADD_NATIVE_WORD(interpreter, "file.r/o", [](auto intr) { intr->push(O_RDONLY); });
        ADD_NATIVE_WORD(interpreter, "file.w/o", [](auto intr) { intr->push(O_WRONLY); });
        ADD_NATIVE_WORD(interpreter, "file.r/w", [](auto intr) { intr->push(O_RDWR); });
    }


}

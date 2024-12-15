
#include "sorth.h"


#if defined (IS_WINDOWS)



namespace sorth
{


    using namespace internal;




    namespace
    {



        HANDLE handle_open(InterpreterPtr interpreter, const std::string& path, DWORD flags,
                           DWORD create_flags)
        {
            DWORD share_flags =   (flags & GENERIC_READ ? FILE_SHARE_READ : 0)
                                | (flags & GENERIC_WRITE ? FILE_SHARE_WRITE : 0);

            HANDLE handle = CreateFileA(path.c_str(),
                                        flags,
                                        share_flags,
                                        nullptr,
                                        create_flags,
                                        FILE_ATTRIBUTE_NORMAL,
                                        nullptr);

            if (handle == INVALID_HANDLE_VALUE)
            {
                throw_windows_error(interpreter, "Could not open file: ", GetLastError());
            }

            return handle;
        }




        char handle_read_char(InterpreterPtr& interpreter, HANDLE handle)
        {
            DWORD bytes_read = 0;
            char buffer = 0;
            auto result = ReadFile(handle, &buffer, 1, &bytes_read, nullptr);

            if (   (result)
                && (bytes_read == 1))
            {
                return buffer;
            }

            throw_windows_error(interpreter, "File read byte failed: ", GetLastError());
        }




        void handle_write_bytes(InterpreterPtr& interpreter,
                                HANDLE handle,
                                const char* buffer,
                                DWORD size)
        {
            BOOL result;
            DWORD bytes_written = 0;
            DWORD total_written = 0;

            do
            {
                result = WriteFile(handle,
                                   &buffer[total_written],
                                   size - total_written,
                                   &bytes_written,
                                   nullptr);

                total_written += bytes_written;
            }
            while ((result) && (total_written < size));

            if (!result)
            {
                throw_windows_error(interpreter, "File write failed: ", GetLastError());
            }
        }




        void word_file_open(InterpreterPtr& interpreter)
        {
            DWORD flags = (DWORD)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto path = as_string(interpreter, interpreter->pop());

            interpreter->push((int64_t)handle_open(interpreter, path, flags, OPEN_ALWAYS));
        }




        void word_file_create_tempfile(InterpreterPtr& interpreter)
        {
            DWORD flags = (DWORD)as_numeric<int64_t>(interpreter, interpreter->pop());
            char temp_path[MAX_PATH + 1] = "";
            char full_temp_path[MAX_PATH + 1] = "";

            auto result = GetTempPathA(MAX_PATH + 1, temp_path);

            throw_windows_error_if(result == 0,
                                   interpreter,
                                   "Failed to get temp path: ",
                                   GetLastError());

            result = GetTempFileNameA(temp_path, "sftmp", 0, full_temp_path);

            throw_windows_error_if(result == 0,
                                   interpreter,
                                   "Failed to get generate temporary file name: ",
                                   GetLastError());

            auto handle = handle_open(interpreter, full_temp_path, flags, CREATE_ALWAYS);

            interpreter->push(std::string(full_temp_path));
            interpreter->push((int64_t)handle);
        }




        void word_file_create(InterpreterPtr& interpreter)
        {
            DWORD flags = (DWORD)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto path = as_string(interpreter, interpreter->pop());

            interpreter->push((int64_t)handle_open(interpreter, path, flags, CREATE_ALWAYS));
        }




        void word_file_close(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());

            if (!CloseHandle(handle))
            {
                throw_windows_error(interpreter, "Could not close handle: ", GetLastError());
            }
        }




        void word_file_delete(InterpreterPtr& interpreter)
        {
            auto path = as_string(interpreter, interpreter->pop());
            auto result = DeleteFileA(path.c_str());

            if (result == FALSE)
            {
                std::string message = "Failed to delete file, " + path + ": ";
                throw_windows_error(interpreter, message, GetLastError());
            }
        }




        void word_socket_connect(InterpreterPtr& interpreter)
        {
            auto pipe_path = as_string(interpreter, interpreter->pop());
            HANDLE pipe;

            do
            {
                pipe = CreateFileA(pipe_path.c_str(),
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   nullptr,
                                   OPEN_EXISTING,
                                   0,
                                   nullptr);
            }
            while (   (pipe == INVALID_HANDLE_VALUE)
                   && (GetLastError() == ERROR_PIPE_BUSY)
                   && (WaitNamedPipeA(pipe_path.c_str(), NMPWAIT_WAIT_FOREVER)));

            if (pipe == INVALID_HANDLE_VALUE)
            {
                throw_windows_error(interpreter, "Could not open pipe: ", GetLastError());
            }

            interpreter->push((int64_t)pipe);
        }




        void word_file_size_read(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());

            LARGE_INTEGER file_size;

            file_size.QuadPart = 0;

            if (!GetFileSizeEx(handle, &file_size))
            {
                throw_windows_error(interpreter, "Get file size failed: ", GetLastError());
            }

            interpreter->push((int64_t)file_size.QuadPart);
        }




        void word_file_exists(InterpreterPtr& interpreter)
        {
            auto file_path = as_string(interpreter, interpreter->pop());
            auto attrib = GetFileAttributesA(file_path.c_str());

            bool result =    (attrib != INVALID_FILE_ATTRIBUTES)
                          && (!(attrib & FILE_ATTRIBUTE_DIRECTORY));

            interpreter->push(result);
        }




        void word_file_is_open(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            DWORD flags = 0;

            if (!GetHandleInformation(handle, &flags))
            {
                interpreter->push(false);
            }

            interpreter->push(true);
        }




        void word_file_is_eof(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            char buffer = 0;
            DWORD bytes_read = 0;

            bool is_eof = false;

            bool result = ReadFile(handle, &buffer, 1, &bytes_read, nullptr);

            if (result)
            {
                if (bytes_read == 0)
                {
                    is_eof = true;
                }
                else
                {
                    SetFilePointer(handle, -1, nullptr, FILE_CURRENT);
                }
            }
            else
            {
                throw_windows_error(interpreter, "File EOF check failed: ", GetLastError());
            }

            interpreter->push(is_eof);
        }




        void word_file_read(InterpreterPtr& interpreter)
        {
            throw_error(interpreter, "This word is currently unimplemented.");
        }




        void word_file_read_character(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            char next_char = handle_read_char(interpreter, handle);

            interpreter->push(std::string(1, next_char));
        }




        void word_file_read_string(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            DWORD size = (DWORD)as_numeric<int64_t>(interpreter, interpreter->pop());

            char* buffer = nullptr;

            try
            {
                buffer = (char*)malloc(size + 1);
                memset(buffer, 0, size + 1);

                BOOL result = FALSE;
                DWORD bytes_total = 0;
                DWORD bytes_read = 0;

                do
                {
                    result = ReadFile(handle,
                                      &buffer[bytes_total],
                                      size - bytes_read,
                                      &bytes_read,
                                      nullptr);

                    bytes_total += bytes_read;
                }
                while ((result) && (bytes_total < (size - 1)));


                if (!result)
                {
                    throw_windows_error(interpreter, "Get file read failed: ", GetLastError());
                }

                interpreter->push(std::string(buffer));

                free(buffer);
            }
            catch (...)
            {
                free(buffer);
                throw;
            }
        }




        void word_file_write(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto value = interpreter->pop();

            if (std::holds_alternative<ByteBufferPtr>(value))
            {
                auto buffer = as_byte_buffer(interpreter, value);

                handle_write_bytes(interpreter, handle, (char*)buffer->data_ptr(),
                                   (DWORD)buffer->size());
            }
            else
            {
                std::stringstream buffer;

                buffer << value;

                handle_write_bytes(interpreter, handle, buffer.str().c_str(),
                                   (DWORD)buffer.str().size());
            }
        }




        void word_file_line_read(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            char next_char;
            std::string line;

            do
            {
                next_char = handle_read_char(interpreter, handle);

                if (next_char != '\n')
                {
                    line += next_char;
                }
            }
            while (next_char != '\n');

            interpreter->push(line);
        }




        void word_file_line_write(InterpreterPtr& interpreter)
        {
            auto handle = (HANDLE)as_numeric<int64_t>(interpreter, interpreter->pop());
            auto line = as_string(interpreter, interpreter->pop());

            if (line[line.size() - 1] != '\n')
            {
                line += '\n';
            }

            handle_write_bytes(interpreter, handle, line.c_str(), (DWORD)line.size());
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

        ADD_NATIVE_WORD(interpreter, "file.create.tempfile", word_file_create_tempfile,
                        "Create/open an unique temporary file and return it's fd.",
                        "flags -- path fd");

        ADD_NATIVE_WORD(interpreter, "file.close", word_file_close,
                        "Take a fd and close it.",
                        "fd -- ");

        ADD_NATIVE_WORD(interpreter, "file.delete", word_file_delete,
                        "Delete the specified file.",
                        "file_path -- ");


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


        ADD_NATIVE_WORD(interpreter, "file.r/o",
            [](auto intr)
            {
                intr->push((int64_t)GENERIC_READ);
            },
            "Constant for opening a file as read only.",
            " -- flag");

        ADD_NATIVE_WORD(interpreter, "file.w/o",
            [](auto intr)
            {
                intr->push((int64_t)GENERIC_WRITE);
            },
            "Constant for opening a file as write only.",
            " -- flag");

        ADD_NATIVE_WORD(interpreter, "file.r/w",
            [](auto intr)
            {
                intr->push((int64_t)GENERIC_READ | GENERIC_WRITE);
            },
            "Constant for opening a file for both reading and writing.",
            " -- flag");
    }


}


#endif // if IS_WINDOWS


#include <iostream>
#include "sorth.h"

#if defined(__APPLE__)

    #include <assert.h>
    #include <mach-o/dyld.h>

#elif defined(__linux__)

    #include <linux/limits.h>
    #include <unistd.h>

#endif




namespace
{


    std::filesystem::path get_executable_directory()
    {
        std::filesystem::path base_path;

        #if defined(__APPLE__)

            uint32_t buffer_size = 0;

            assert(_NSGetExecutablePath(nullptr, &buffer_size) == -1);

            std::vector<char> buffer((size_t)buffer_size);

            assert(_NSGetExecutablePath(&buffer[0], &buffer_size) == 0);

            base_path = std::filesystem::canonical(&buffer[0]).remove_filename();

        #elif defined(__linux__)

            char buffer [PATH_MAX + 1];
            ssize_t count = 0;

            memset(buffer, 0, PATH_MAX + 1);

            count = readlink("/proc/self/exe", buffer, PATH_MAX);

            if (count < 0)
            {
                throw std::runtime_error("Executable path could not be read, " +
                                         std::string(strerror(errno)) + ".");
            }

            base_path = std::filesystem::canonical(buffer).remove_filename();

        #elif defined(IS_WINDOWS)

            char buffer [ MAX_PATH + 1];

            memset(buffer, 0, MAX_PATH + 1);

            GetModuleFileNameA(nullptr, buffer, MAX_PATH);

            if (GetLastError() != ERROR_SUCCESS)
            {
                char message_buffer[4096];
                memset(message_buffer, 0, 4096);
                size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                             FORMAT_MESSAGE_FROM_SYSTEM |
                                             FORMAT_MESSAGE_IGNORE_INSERTS,
                                             nullptr,
                                             GetLastError(),
                                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                             message_buffer,
                                             0,
                                             NULL);
                std::stringstream stream;

                stream << "Could not get the executable directory: " << message_buffer;

                throw std::runtime_error(stream.str());
            }

            base_path = std::filesystem::canonical(buffer).remove_filename();

        #else

            throw std::runtime_error("get_executable_directory is unimplemented on this platform.");

        #endif

        return base_path;
    }


}



int main(int argc, char* argv[])
{
    int exit_code;

    try
    {
        auto interpreter = sorth::create_interpreter();

        interpreter->add_search_path(get_executable_directory());

        sorth::register_builtin_words(interpreter);
        sorth::register_terminal_words(interpreter);
        sorth::register_io_words(interpreter);
        sorth::register_user_words(interpreter);
        sorth::register_ffi_words(interpreter);

        auto std_lib = interpreter->find_file("std.f");
        interpreter->process_source(std_lib);

        interpreter->mark_context();

        interpreter->add_search_path(std::filesystem::current_path());

        if (argc >= 2)
        {
            sorth::ArrayPtr array = std::make_shared<sorth::Array>(argc - 2);

            for (int i = 0; i < argc - 2; ++i)
            {
                (*array)[i] = std::string(argv[i + 2]);
            }

            ADD_NATIVE_WORD(interpreter,
                "args",
                [array](auto interpreter)
                {
                    interpreter->push(array);
                },
                "List of command line arguments passed to the script.",
                " -- arguments");

            auto user_source_path = interpreter->find_file(argv[1]);
            interpreter->process_source(user_source_path);
        }
        else
        {
            interpreter->execute_word("repl");
        }

        exit_code = interpreter->get_exit_code();
    }
    catch(const std::runtime_error& error)
    {
        std::cerr << "Run-Time error: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    return exit_code;
}

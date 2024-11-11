
// The main entry point for the Strange Forth, (or sorth,) interpreter.  This is where the command
// line arguments are processed and the interpreter is set up to run the script or the REPL.
//
// The standard library is found in either the directory that the executable is in, or it can be
// specified by the SORTH_LIB environment variable.
//
// This project is covered by the MIT License.  See the file "LICENSE" in the project root for more
// information.

#if defined(__APPLE__)

    #include <assert.h>
    #include <mach-o/dyld.h>

#elif defined(__linux__)

    #include <linux/limits.h>
    #include <unistd.h>

#endif

// Disable the CRT secure warnings for the deprecation warning for the function std::get_env on
// Windows.
#define _CRT_SECURE_NO_WARNINGS 1
#include <cstdlib>
#undef _CRT_SECURE_NO_WARNINGS

#include "sorth.h"



namespace
{


    // Get the directory where the executable is located.  This is used to find the standard library
    // in the case the user hasn't specified another location for it.
    std::filesystem::path get_executable_directory()
    {
        std::filesystem::path base_path;

        // Refer to OS specific means to get the current executable's path.
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


    // Get the directory where the standard library is located.  This can either be the directory
    // that the executable is in, or it can be specified by the SORTH_LIB environment variable.
    std::filesystem::path get_std_lib_directory()
    {
        auto env_path = std::getenv("SORTH_LIB");

        if (env_path != nullptr)
        {
            return std::filesystem::canonical(env_path);
        }

        return get_executable_directory();
    }


}



// Let's get this interpreter started!
int main(int argc, char* argv[])
{
    // The exit code that we'll return to the operating system.
    int exit_code = EXIT_SUCCESS;

    try
    {
        // Create the interpreter and set up the search path to be able to find the standard
        // library.
        auto interpreter = sorth::create_interpreter();

        interpreter->add_search_path(get_executable_directory());

        // Register all of the built-in words.
        sorth::register_builtin_words(interpreter);
        sorth::register_terminal_words(interpreter);
        sorth::register_io_words(interpreter);
        sorth::register_user_words(interpreter);
        sorth::register_ffi_words(interpreter);

        // Load the standard library to augment the built-in words.
        auto std_lib = interpreter->find_file("std.f");
        interpreter->process_source(std_lib);

        // Mark this context as a known good starting point.  If we need to reset the interpreter it
        // will be reset to this point.
        interpreter->mark_context();

        // Add the current directory to the search path.
        interpreter->add_search_path(std::filesystem::current_path());

        // Check to see if the user requested that we run a specific script.
        if (argc >= 2)
        {
            // Looks like we have a script to run.  Load up any remaining command line arguments
            // into an array and make them available to the script as the word sorth.args.
            sorth::ArrayPtr array = std::make_shared<sorth::Array>(argc - 2);

            for (int i = 0; i < argc - 2; ++i)
            {
                (*array)[i] = std::string(argv[i + 2]);
            }

            ADD_NATIVE_WORD(interpreter,
                "sorth.args",
                [array](auto interpreter)
                {
                    interpreter->push(array);
                },
                "List of command line arguments passed to the script.",
                " -- arguments");

            // Find the script file and run it.
            auto user_source_path = interpreter->find_file(argv[1]);
            interpreter->process_source(user_source_path);
        }
        else
        {
            // There was no script to run, so we'll just start the REPL from the standard library.
            interpreter->execute_word("repl");
        }

        // Get the exit code from the interpreter so that we can return it to the operating system.
        exit_code = interpreter->get_exit_code();
    }
    catch(const std::runtime_error& error)
    {
        // Something went wrong, so we'll print out the error and return a failure code.
        std::cerr << "Run-Time error: " << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    return exit_code;
}

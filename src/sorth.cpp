
#include "sorth.h"

#ifdef __APPLE__

    #include <assert.h>
    #include <mach-o/dyld.h>

#endif



namespace
{


    std::filesystem::path get_executable_directory()
    {
        std::filesystem::path base_path;

        #ifdef __APPLE__

            uint32_t buffer_size = 0;

            assert(_NSGetExecutablePath(nullptr, &buffer_size) == -1);

            std::vector<char> buffer((size_t)buffer_size);

            assert(_NSGetExecutablePath(&buffer[0], &buffer_size) == 0);

            base_path = std::filesystem::canonical(&buffer[0]).remove_filename();

        #elif

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
        sorth::register_io_words(interpreter);

        auto std_lib = interpreter->find_file("std.f");
        interpreter->process_source(std_lib);

        interpreter->mark_context();

        interpreter->add_search_path(std::filesystem::current_path());

        if (argc == 2)
        {
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

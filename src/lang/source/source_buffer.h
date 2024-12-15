
#pragma once


namespace sorth::internal
{


    // Helper class that keeps track of the current location as we tokenize the source code.  It'll
    // also take care of loading the source from a file on disk.  Or just simply accept text from
    // the REPL.

    class SourceBuffer
    {
        private:
            std::string source;
            size_t position;
            Location source_location;

        public:
            SourceBuffer();
            SourceBuffer(std::string const& new_source);
            SourceBuffer(const std::filesystem::path& path);

        public:
            operator bool() const;

        public:
            char peek_next() const;
            char next();

            Location current_location() const;

        private:
            void increment_position(char next);
    };


}

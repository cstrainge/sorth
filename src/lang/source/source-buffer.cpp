
#include "sorth.h"



namespace sorth::internal
{


    SourceBuffer::SourceBuffer()
    : source(),
      position(0),
      source_location("<unknown>")
    {
    }

    SourceBuffer::SourceBuffer(const std::string& name, std::string const& new_source)
    : source(new_source),
      position(0),
      source_location(name)
    {
    }

    SourceBuffer::SourceBuffer(const std::filesystem::path& path)
    : source(load_source(path)),
      position(0),
      source_location(path.string())
    {
    }


    SourceBuffer::operator bool() const
    {
        return position < source.size();
    }


    char SourceBuffer::peek_next() const
    {
        if (position >= source.size())
        {
            return ' ';
        }

        return source[position];
    }

    char SourceBuffer::next()
    {
        auto next = peek_next();

        if (*this)
        {
            increment_position(next);
        }

        return next;
    }

    Location SourceBuffer::current_location() const
    {
        return source_location;
    }


    void SourceBuffer::increment_position(char next)
    {
        ++position;

        if (next == '\n')
        {
            source_location.next_line();
        }
        else
        {
            source_location.next();
        }
    }


    std::string SourceBuffer::load_source(const std::filesystem::path& path)
    {
        std::ifstream source_file(path.c_str());

        auto begin = std::istreambuf_iterator<char>(source_file);
        auto end = std::istreambuf_iterator<char>();

        return std::string(begin, end);
    }


}

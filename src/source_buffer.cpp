
#include "sorth.h"



namespace sorth::internal
{


    SourceBuffer::SourceBuffer()
    : source(),
      position(0),
      source_location(shared_path("<repl>"))
    {
    }

    SourceBuffer::SourceBuffer(std::string const& new_source)
    : source(new_source),
      position(0),
      source_location(shared_path("<repl>"))
    {
    }

    SourceBuffer::SourceBuffer(const std::filesystem::path& path)
    : source(),
      position(0),
      source_location(shared_path(path))
    {
        std::ifstream source_file(*source_location.get_path());

        auto begin = std::istreambuf_iterator<char>(source_file);
        auto end = std::istreambuf_iterator<char>();

        source = std::string(begin, end);

        // TODO: Look for a #! on the first line and remove it before the rest of the
        //       interpreter sees it.
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


}

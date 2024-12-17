
#pragma once


namespace sorth::internal
{


    class Location
    {
        private:
            std::string source_path;
            size_t line;
            size_t column;

        public:
            Location();
            Location(const std::string& path);
            Location(const std::string& path, size_t new_line, size_t new_column);

        public:
            const std::string& get_path() const;
            size_t get_line() const;
            size_t get_column() const;

        private:
            void next();
            void next_line();

            friend class SourceBuffer;
    };


    #define LOCATION_HERE() \
        sorth::internal::Location(__FILE__, __LINE__, 1)


    std::ostream& operator <<(std::ostream& stream, Location location);


    std::strong_ordering operator <=>(const Location& lhs, const Location& rhs);


    inline bool operator ==(const Location& lhs, const Location& rhs)
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }


    inline bool operator !=(const Location& lhs, const Location& rhs)
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }


}

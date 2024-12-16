
#pragma once


namespace sorth::internal
{


    using PathPtr = std::shared_ptr<std::filesystem::path>;


    inline PathPtr shared_path(const char* string)
    {
        return std::make_shared<std::filesystem::path>(string);
    }


    inline PathPtr shared_path(const std::string& string)
    {
        return std::make_shared<std::filesystem::path>(string);
    }


    inline PathPtr shared_path(const std::filesystem::path& path)
    {
        return std::make_shared<std::filesystem::path>(path);
    }


    class Location
    {
        private:
            PathPtr source_path;
            size_t line;
            size_t column;

        public:
            Location();
            Location(const PathPtr& path);
            Location(const PathPtr& path, size_t new_line, size_t new_column);

        public:
            const PathPtr& get_path() const;
            size_t get_line() const;
            size_t get_column() const;

        public:
            void next();
            void next_line();
    };


    #define LOCATION_HERE() \
        sorth::internal::Location(shared_path(__FILE__), __LINE__, 0)



    std::ostream& operator <<(std::ostream& stream, Location location);

    bool operator ==(const Location& rhs, const Location& lhs);


}

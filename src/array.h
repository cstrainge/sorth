
#pragma once


namespace sorth
{


    class Array
    {
        private:
            std::vector<Value> items;

        public:
            Array(int64_t size);

        public:
            int64_t size() const;
            Value& operator [](int64_t index);
            void resize(int64_t new_size);

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);
    };


    std::ostream& operator <<(std::ostream& stream, const ArrayPtr& array);


}

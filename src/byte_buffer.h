
#pragma once


namespace sorth
{


    class ByteBuffer
    {
        private:
            std::vector<unsigned char> bytes;
            int64_t current_position;

        public:
            ByteBuffer(int64_t size);
            ByteBuffer(const ByteBuffer& buffer);
            ByteBuffer(ByteBuffer&& buffer);

        public:
            ByteBuffer& operator =(const ByteBuffer& buffer);
            ByteBuffer& operator =(ByteBuffer&& buffer);

        public:
            void resize(int64_t new_size);
            int64_t size() const;

            int64_t position() const;
            void* position_ptr() const;

            void set_position(int64_t new_position);

        public:
            void* data_ptr();

        public:
            void write_int(int64_t byte_size, int64_t value);
            int64_t read_int(int64_t byte_size, bool is_signed);

            void write_float(int64_t byte_size, double value);
            double read_float(int64_t byte_size);

            void write_string(const std::string& string, int64_t max_size);
            std::string read_string(int64_t max_size);

        private:
            void increment_position(int64_t increment);

        private:
            friend std::ostream& operator <<(std::ostream& stream, const ByteBuffer& buffer);
    };


    std::ostream& operator <<(std::ostream& stream, const ByteBuffer& buffer);

    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr);


    bool operator ==(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs);


}

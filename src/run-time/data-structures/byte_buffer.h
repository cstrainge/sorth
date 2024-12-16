
#pragma once


namespace sorth
{


    class Buffer
    {
        public:
            Buffer();
            virtual ~Buffer();

        public:
            virtual void resize(size_t new_size) = 0;
            virtual size_t size() const = 0;

            virtual size_t position() const = 0;
            virtual void* position_ptr() const = 0;

            virtual void set_position(size_t new_position) = 0;
            virtual void increment_position(size_t increment) = 0;

        public:
            virtual void* data_ptr() = 0;
            virtual const void* data_ptr() const = 0;

        public:
            virtual void write_int(size_t byte_size, int64_t value) = 0;
            virtual int64_t read_int(size_t byte_size, bool is_signed) = 0;

            virtual void write_float(size_t byte_size, double value) = 0;
            virtual double read_float(size_t byte_size) = 0;

            virtual void write_string(const std::string& string, size_t max_size) = 0;
            virtual std::string read_string(size_t max_size) = 0;

        public:
            virtual size_t hash() const noexcept
            {
                size_t hash_value = 0;
                const char* buffer = static_cast<const char*>(data_ptr());

                for (size_t i = 0; i < size(); ++i)
                {
                    hash_value ^= Value::hash_combine(hash_value, std::hash<char>()(buffer[i]));
                }

                return hash_value;
            }
    };



    std::ostream& operator <<(std::ostream& stream, const Buffer& buffer);



    class ByteBuffer : public Buffer
    {
        private:
            bool owned;
            unsigned char* bytes;
            size_t byte_size;

            size_t current_position;

        public:
            ByteBuffer(size_t size);
            ByteBuffer(void* raw_ptr, size_t size);
            ByteBuffer(const ByteBuffer& buffer);
            ByteBuffer(ByteBuffer&& buffer);
            virtual ~ByteBuffer() override;

        public:
            ByteBuffer& operator =(const ByteBuffer& buffer);
            ByteBuffer& operator =(ByteBuffer&& buffer);

        public:
            virtual void resize(size_t new_size) override;
            virtual size_t size() const override;

            virtual size_t position() const override;
            virtual void* position_ptr() const override;

            virtual void set_position(size_t new_position) override;
            virtual void increment_position(size_t increment) override;

        public:
            virtual void* data_ptr() override;
            virtual const void* data_ptr() const override;

        public:
            virtual void write_int(size_t byte_size, int64_t value) override;
            virtual int64_t read_int(size_t byte_size, bool is_signed) override;

            virtual void write_float(size_t byte_size, double value) override;
            virtual double read_float(size_t byte_size) override;

            virtual void write_string(const std::string& string, size_t max_size) override;
            virtual std::string read_string(size_t max_size) override;

        private:
            void reset();
    };


    std::strong_ordering operator <=>(const ByteBuffer& rhs, const ByteBuffer& lhs);

    std::strong_ordering operator <=>(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs);


    inline bool operator ==(const ByteBuffer& rhs, const ByteBuffer& lhs)
    {
        return (rhs <=> lhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const ByteBuffer& rhs, const ByteBuffer& lhs)
    {
        return (rhs <=> lhs) != std::strong_ordering::equal;
    }

    inline bool operator ==(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs)
    {
        return *rhs == *lhs;
    }

    inline bool operator !=(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs)
    {
        return *rhs == *lhs;
    }


    class SubBuffer : public Buffer
    {
        private:
            Buffer& parent;
            size_t base_position;
            size_t current_position;

        public:
            SubBuffer(Buffer& parent, size_t base_position);
            virtual ~SubBuffer();

        public:
            virtual void resize(size_t new_size) override;
            virtual size_t size() const override;

            virtual size_t position() const override;
            virtual void* position_ptr() const override;

            virtual void set_position(size_t new_position) override;
            virtual void increment_position(size_t increment) override;

        public:
            virtual void* data_ptr() override;
            virtual const void* data_ptr() const override;

        public:
            virtual void write_int(size_t byte_size, int64_t value) override;
            virtual int64_t read_int(size_t byte_size, bool is_signed) override;

            virtual void write_float(size_t byte_size, double value) override;
            virtual double read_float(size_t byte_size) override;

            virtual void write_string(const std::string& string, size_t max_size) override;
            virtual std::string read_string(size_t max_size) override;

        private:
            void local_increment_position(size_t increment);
    };


    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr);


    bool operator ==(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs);


}


#pragma once


namespace sorth
{


    class Buffer
    {
        public:
            Buffer();
            virtual ~Buffer();

        public:
            virtual void resize(int64_t new_size) = 0;
            virtual int64_t size() const = 0;

            virtual int64_t position() const = 0;
            virtual void* position_ptr() const = 0;

            virtual void set_position(int64_t new_position) = 0;

        public:
            virtual void* data_ptr() = 0;
            virtual const void* data_ptr() const = 0;

        public:
            virtual void write_int(int64_t byte_size, int64_t value) = 0;
            virtual int64_t read_int(int64_t byte_size, bool is_signed) = 0;

            virtual void write_float(int64_t byte_size, double value) = 0;
            virtual double read_float(int64_t byte_size) = 0;

            virtual void write_string(const std::string& string, int64_t max_size) = 0;
            virtual std::string read_string(int64_t max_size) = 0;
    };



    std::ostream& operator <<(std::ostream& stream, const Buffer& buffer);



    class ByteBuffer : public Buffer
    {
        private:
            bool owned;
            unsigned char* bytes;
            int64_t byte_size;

            int64_t current_position;

        public:
            ByteBuffer(int64_t size);
            ByteBuffer(void* raw_ptr, int64_t size);
            ByteBuffer(const ByteBuffer& buffer);
            ByteBuffer(ByteBuffer&& buffer);
            virtual ~ByteBuffer() override;

        public:
            ByteBuffer& operator =(const ByteBuffer& buffer);
            ByteBuffer& operator =(ByteBuffer&& buffer);

        public:
            virtual void resize(int64_t new_size) override;
            virtual int64_t size() const override;

            virtual int64_t position() const override;
            virtual void* position_ptr() const override;

            virtual void set_position(int64_t new_position) override;

        public:
            virtual void* data_ptr() override;
            virtual const void* data_ptr() const override;

        public:
            virtual void write_int(int64_t byte_size, int64_t value) override;
            virtual int64_t read_int(int64_t byte_size, bool is_signed) override;

            virtual void write_float(int64_t byte_size, double value) override;
            virtual double read_float(int64_t byte_size) override;

            virtual void write_string(const std::string& string, int64_t max_size) override;
            virtual std::string read_string(int64_t max_size) override;

        private:
            void increment_position(int64_t increment);
            void reset();
    };



    class SubBuffer : public Buffer
    {
        private:
            Buffer& parent;
            int64_t base_position;
            int64_t current_position;

        public:
            SubBuffer(Buffer& parent, int64_t base_position);
            virtual ~SubBuffer();

        public:
            virtual void resize(int64_t new_size) override;
            virtual int64_t size() const override;

            virtual int64_t position() const override;
            virtual void* position_ptr() const override;

            virtual void set_position(int64_t new_position) override;

        public:
            virtual void* data_ptr() override;
            virtual const void* data_ptr() const override;

        public:
            virtual void write_int(int64_t byte_size, int64_t value) override;
            virtual int64_t read_int(int64_t byte_size, bool is_signed) override;

            virtual void write_float(int64_t byte_size, double value) override;
            virtual double read_float(int64_t byte_size) override;

            virtual void write_string(const std::string& string, int64_t max_size) override;
            virtual std::string read_string(int64_t max_size) override;

        private:
            void increment_position(int64_t increment);
    };


    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr);


    bool operator ==(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs);


}

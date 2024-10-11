
#include "sorth.h"



namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const ByteBuffer& buffer)
    {
        auto byte_string = [&](size_t start, size_t stop)
            {
                if (stop == 0)
                {
                    return;
                }

                auto spaces = ((16 - (stop - start)) * 3);

                for (size_t i = 0; i <= spaces; ++i)
                {
                    stream << " ";
                }

                for (size_t i = start; i < stop; ++i)
                {
                    unsigned char next = buffer.bytes[i];
                    bool is_ctrl = (iscntrl(next) != 0) || ((next & 0x80) != 0);

                    stream << (char)(is_ctrl ? '.' : next);
                }
            };

        stream << "          00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f";

        for (size_t i = 0; i < buffer.bytes.size(); ++i)
        {
            if ((i == 0) || ((i % 16) == 0))
            {
                byte_string(i - 16, i);

                stream << std::endl << std::setw(8) << std::setfill('0') << std::hex
                       << i << "  ";
            }

            stream << std::setw(2) << std::setfill('0') << std::hex
                   << (uint32_t)buffer.bytes[i] << " ";
        }

        stream << std::dec << std::setfill(' ');

        auto left_over = buffer.bytes.size() % 16;

        if (left_over != 0)
        {
            auto index = buffer.bytes.size() - left_over;
            byte_string(index, buffer.bytes.size());
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr)
    {
        stream << *buffer_ptr;
        return stream;
    }


    bool operator ==(const ByteBufferPtr& rhs, const ByteBufferPtr& lhs)
    {
        if (rhs->size() != lhs->size())
        {
            return false;
        }

        return std::memcmp(rhs->data_ptr(), lhs->data_ptr(), rhs->size()) == 0;
    }


    ByteBuffer::ByteBuffer(int64_t size)
    : current_position(0)
    {
        bytes.resize(size);
    }


    ByteBuffer::ByteBuffer(const ByteBuffer& buffer)
    : bytes(buffer.bytes),
      current_position(buffer.current_position)
    {
    }


    ByteBuffer::ByteBuffer(ByteBuffer&& buffer)
    : bytes(std::move(buffer.bytes)),
      current_position(buffer.current_position)
    {
    }


    ByteBuffer& ByteBuffer::operator =(const ByteBuffer& buffer)
    {
        if (&buffer != this)
        {
            bytes = buffer.bytes;
            current_position = buffer.current_position;
        }

        return *this;
    }


    ByteBuffer& ByteBuffer::operator =(ByteBuffer&& buffer)
    {
        if (&buffer != this)
        {
            bytes = std::move(buffer.bytes);
            current_position = buffer.current_position;
        }

        return *this;
    }


    void ByteBuffer::resize(int64_t new_size)
    {
        bytes.resize(new_size);

        if (current_position >= new_size)
        {
            current_position = new_size - 1;
        }
    }


    int64_t ByteBuffer::size() const
    {
        return bytes.size();
    }


    int64_t ByteBuffer::position() const
    {
        return current_position;
    }


    void* ByteBuffer::position_ptr() const
    {
        return (void*)&bytes[current_position];
    }


    void ByteBuffer::set_position(int64_t new_position)
    {
        current_position = new_position;
    }


    void* ByteBuffer::data_ptr()
    {
        return bytes.data();
    }


    void ByteBuffer::write_int(int64_t byte_size, int64_t value)
    {
        void* data_ptr = position_ptr();
        memcpy(data_ptr, &value, byte_size);

        increment_position(byte_size);
    }


    int64_t ByteBuffer::read_int(int64_t byte_size, bool is_signed)
    {
        int64_t value = 0;
        void* data_ptr = position_ptr();
        memcpy(&value, data_ptr, byte_size);

        if (is_signed)
        {
            auto bit_size = byte_size * 8;
            auto sign_flag = 1 << (bit_size - 1);

            if ((value & sign_flag) != 0)
            {
                uint64_t negative_bits = 0xffffffffffffffff << (64 - bit_size);
                value = value | negative_bits;
            }
        }

        increment_position(byte_size);

        return value;
    }


    void ByteBuffer::write_float(int64_t byte_size, double value)
    {
        void* data_ptr = position_ptr();

        if (byte_size == 4)
        {
            float float_value = static_cast<float>(value);

            memcpy(data_ptr, &float_value, 4);
        }
        else
        {
            memcpy(data_ptr, &value, 8);
        }

        increment_position(byte_size);
    }


    double ByteBuffer::read_float(int64_t byte_size)
    {
        double new_value = 0.0;
        void* data_ptr = position_ptr();

        if (byte_size == 4)
        {
            float float_value = 0.0;

            memcpy(&float_value, data_ptr, 4);
            new_value = float_value;
        }
        else
        {
            memcpy(&new_value, data_ptr, 8);
        }

        increment_position(byte_size);

        return new_value;
    }


    void ByteBuffer::write_string(const std::string& string, int64_t max_size)
    {
        void* data_ptr = position_ptr();

        strncpy(static_cast<char*>(data_ptr), string.c_str(), max_size);
        increment_position(max_size);
    }


    std::string ByteBuffer::read_string(int64_t max_size)
    {
        std::string new_string;
        void* data_ptr = nullptr;
        char* buffer = nullptr;

        try
        {
            data_ptr = position_ptr();
            buffer = static_cast<char*>(malloc(max_size + 1));

            memset(buffer, 0, max_size + 1);
            memcpy(buffer, data_ptr, max_size);

            new_string = buffer;
            increment_position(max_size);

            free(buffer);
        }
        catch (...)
        {
            if (buffer)
            {
                free(buffer);
            }

            throw;
        }

        return new_string;
    }



    void ByteBuffer::increment_position(int64_t increment)
    {
        size_t new_position = current_position + increment;

        if (new_position > bytes.size())
        {
            std::stringstream stream;

            stream << "ByteBuffer position " << new_position << " out of range, " << bytes.size()
                   << ".";

            throw std::runtime_error(stream.str());
        }

        current_position = new_position;
    }


}

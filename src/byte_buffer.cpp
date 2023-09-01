
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
                    char next = buffer.bytes[i];
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


    ByteBuffer::ByteBuffer(int64_t size)
    : position(0)
    {
        bytes.resize(size);
    }


    int64_t ByteBuffer::size() const
    {
        return bytes.size();
    }


    int64_t ByteBuffer::postion() const
    {
        return position;
    }


    void ByteBuffer::set_position(int64_t new_position)
    {
        position = new_position;
    }


    void* ByteBuffer::data_ptr()
    {
        return (&bytes[0]);
    }


    void ByteBuffer::write_int(int64_t byte_size, int64_t value)
    {
        void* data_ptr =(&bytes[position]);
        memcpy(data_ptr, &value, byte_size);

        position += byte_size;
    }


    int64_t ByteBuffer::read_int(int64_t byte_size, bool is_signed)
    {
        int64_t value = 0;
        void* data_ptr =(&bytes[position]);
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

        position += byte_size;

        return value;
    }


    void ByteBuffer::write_float(int64_t byte_size, double value)
    {
        void* data_ptr =(&bytes[position]);

        if (byte_size == 4)
        {
            float float_value = (float)value;

            memcpy(data_ptr, &float_value, 4);
        }
        else
        {
            memcpy(data_ptr, &value, 8);
        }

        position += byte_size;
    }


    double ByteBuffer::read_float(int64_t byte_size)
    {
        double new_value = 0.0;
        void* data_ptr =(&bytes[position]);

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

        position += byte_size;

        return new_value;
    }


    void ByteBuffer::write_string(const std::string& string, int64_t max_size)
    {
        void* data_ptr =(&bytes[position]);

        strncpy((char*)data_ptr, string.c_str(), max_size);
        position += max_size;
    }


    std::string ByteBuffer::read_string(int64_t max_size)
    {
        void* data_ptr =(&bytes[position]);
        char buffer[max_size + 1];

        memset(buffer, 0, max_size + 1);
        memcpy(buffer, data_ptr, max_size);

        std::string new_string = buffer;
        position += max_size;

        return new_string;
    }


}

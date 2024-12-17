
#include "sorth.h"


#ifdef min
    #undef min
#endif



namespace sorth
{


    std::ostream& operator <<(std::ostream& stream, const Buffer& buffer)
    {
        auto data_ptr = static_cast<const unsigned char*>(buffer.data_ptr());

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
                    auto next = data_ptr[i];
                    bool is_ctrl = (iscntrl(next) != 0) || ((next & 0x80) != 0);

                    stream << (char)(is_ctrl ? '.' : next);
                }
            };

        stream << "          00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f";

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            if ((i == 0) || ((i % 16) == 0))
            {
                byte_string(i - 16, i);

                stream << std::endl << std::setw(8) << std::setfill('0') << std::hex
                       << i << "  ";
            }

            stream << std::setw(2) << std::setfill('0') << std::hex
                   << (uint32_t)data_ptr[i] << " ";
        }

        stream << std::dec << std::setfill(' ');

        auto left_over = buffer.size() % 16;

        if (left_over != 0)
        {
            auto index = buffer.size() - left_over;
            byte_string(index, buffer.size());
        }

        return stream;
    }


    std::ostream& operator <<(std::ostream& stream, const ByteBufferPtr& buffer_ptr)
    {
        stream << *buffer_ptr;
        return stream;
    }


    Buffer::Buffer()
    {
    }


    Buffer::~Buffer()
    {
    }


    std::strong_ordering operator <=>(const ByteBuffer& lhs, const ByteBuffer& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return lhs.size() <=> rhs.size();
        }

        return std::memcmp(lhs.data_ptr(), rhs.data_ptr(), lhs.size()) <=> 0;
    }


    std::strong_ordering operator <=>(const ByteBufferPtr& lhs, const ByteBufferPtr& rhs)
    {
        return *lhs <=> *rhs;
    }


    ByteBuffer::ByteBuffer(size_t new_size)
    : owned(true),
      bytes(new unsigned char[new_size]),
      byte_size(new_size),
      current_position(0)
    {
        memset(bytes, 0, byte_size);
    }


    ByteBuffer::ByteBuffer(void* raw_ptr, size_t size)
    : owned(false),
      bytes(reinterpret_cast<unsigned char*>(raw_ptr)),
      byte_size(size),
      current_position(0)
    {
    }


    ByteBuffer::ByteBuffer(const ByteBuffer& buffer)
    : owned(true),
      bytes(new unsigned char[buffer.byte_size]),
      byte_size(buffer.byte_size),
      current_position(buffer.current_position)
    {
        memcpy(bytes, buffer.bytes, byte_size);
    }


    ByteBuffer::ByteBuffer(ByteBuffer&& buffer)
    : owned(buffer.owned),
      bytes(buffer.bytes),
      byte_size(buffer.byte_size),
      current_position(buffer.current_position)
    {
        buffer.owned = false;
        buffer.bytes = nullptr;
        buffer.byte_size = 0;
        buffer.current_position = 0;
    }


    ByteBuffer::~ByteBuffer()
    {
        reset();
    }


    ByteBuffer& ByteBuffer::operator =(const ByteBuffer& buffer)
    {
        if (&buffer != this)
        {
            reset();

            owned = true;
            bytes = new unsigned char[buffer.byte_size];
            byte_size = buffer.byte_size;
            current_position = buffer.current_position;
        }

        return *this;
    }


    ByteBuffer& ByteBuffer::operator =(ByteBuffer&& buffer)
    {
        if (&buffer != this)
        {
            reset();

            owned = buffer.owned;
            bytes = buffer.bytes;
            byte_size = buffer.byte_size;
            current_position = buffer.current_position;

            buffer.owned = false;
            buffer.reset();
        }

        return *this;
    }


    void ByteBuffer::resize(size_t new_size)
    {
        if (owned == false)
        {
            throw std::runtime_error("Resizing a byte buffer that isn't owned.");
        }

        auto new_buffer = new unsigned char[new_size];

        memcpy(new_buffer, bytes, std::min(new_size, byte_size));
        delete [] bytes;

        byte_size = new_size;
        bytes = new_buffer;

        memset(bytes, 0, byte_size);

        if (current_position >= new_size)
        {
            current_position = new_size - 1;
        }
    }


    size_t ByteBuffer::size() const
    {
        return byte_size;
    }


    size_t ByteBuffer::position() const
    {
        return current_position;
    }


    void* ByteBuffer::position_ptr() const
    {
        return (void*)&bytes[current_position];
    }


    void ByteBuffer::set_position(size_t new_position)
    {
        current_position = new_position;
    }


    void ByteBuffer::increment_position(size_t increment)
    {
        auto new_position = current_position + increment;

        if (   (new_position > byte_size)
            && (byte_size != -1))
        {
            std::stringstream stream;

            stream << "ByteBuffer position " << new_position << " out of range, " << byte_size
                   << ".";

            throw std::runtime_error(stream.str());
        }

        current_position = new_position;
    }


    void* ByteBuffer::data_ptr()
    {
        return bytes;
    }


    const void* ByteBuffer::data_ptr() const
    {
        return bytes;
    }


    void ByteBuffer::write_int(size_t byte_size, int64_t value)
    {
        void* data_ptr = position_ptr();
        memcpy(data_ptr, &value, byte_size);

        increment_position(byte_size);
    }


    int64_t ByteBuffer::read_int(size_t byte_size, bool is_signed)
    {
        size_t value = 0;
        void* data_ptr = position_ptr();
        memcpy(&value, data_ptr, byte_size);

        if (is_signed)
        {
            auto bit_size = byte_size * 8;
            auto sign_flag = 1 << (bit_size - 1);

            if ((value & sign_flag) != 0)
            {
                size_t negative_bits = 0xffffffffffffffff << (64 - bit_size);
                value = value | negative_bits;
            }
        }

        increment_position(byte_size);

        return value;
    }


    void ByteBuffer::write_float(size_t byte_size, double value)
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


    double ByteBuffer::read_float(size_t byte_size)
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


    void ByteBuffer::write_string(const std::string& string, size_t max_size)
    {
        void* data_ptr = position_ptr();

        strncpy(static_cast<char*>(data_ptr), string.c_str(), max_size);
        increment_position(max_size);
    }


    std::string ByteBuffer::read_string(size_t max_size)
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


    void ByteBuffer::reset()
    {
        if (   (owned)
            && (bytes != nullptr))
        {
            delete []bytes;
        }

        bytes = nullptr;
        byte_size = 0;
        current_position = 0;
    }




    SubBuffer::SubBuffer(Buffer& parent, size_t base_position)
    : parent(parent),
      base_position(base_position)
    {
    }


    SubBuffer::~SubBuffer()
    {
    }


    void SubBuffer::resize(size_t new_size)
    {
        parent.resize(new_size + base_position);
    }


    size_t SubBuffer::size() const
    {
        return parent.size() - base_position;
    }


    size_t SubBuffer::position() const
    {
        return current_position;
    }


    void* SubBuffer::position_ptr() const
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        auto result = parent.position_ptr();
        parent.set_position(original);

        return result;
    }


    void SubBuffer::set_position(size_t new_position)
    {
        if (new_position > size())
        {
            std::stringstream stream;

            stream << "SubBuffer position " << new_position << " out of range, " << size()
                   << ".";

            throw std::runtime_error(stream.str());
        }

        current_position = new_position;
    }


    void SubBuffer::increment_position(size_t increment)
    {
        parent.increment_position(increment);
    }


    void* SubBuffer::data_ptr()
    {
        auto original = parent.position();

        parent.set_position(base_position);
        auto result = parent.position_ptr();
        parent.set_position(original);

        return result;
    }


    const void* SubBuffer::data_ptr() const
    {
        auto original = parent.position();

        parent.set_position(base_position);
        auto result = parent.position_ptr();
        parent.set_position(original);

        return result;
    }


    void SubBuffer::write_int(size_t byte_size, int64_t value)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        parent.write_int(byte_size, value);
        parent.set_position(original);

        local_increment_position(byte_size);
    }


    int64_t SubBuffer::read_int(size_t byte_size, bool is_signed)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        auto result = parent.read_int(byte_size, is_signed);
        parent.set_position(original);

        local_increment_position(byte_size);

        return result;
    }


    void SubBuffer::write_float(size_t byte_size, double value)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        parent.write_float(byte_size, value);
        parent.set_position(original);

        local_increment_position(byte_size);
    }


    double SubBuffer::read_float(size_t byte_size)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        auto result = parent.read_float(byte_size);
        parent.set_position(original);

        local_increment_position(byte_size);

        return result;
    }


    void SubBuffer::write_string(const std::string& string, size_t max_size)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        parent.write_string(string, max_size);
        parent.set_position(original);

        local_increment_position(max_size);
    }


    std::string SubBuffer::read_string(size_t max_size)
    {
        auto original = parent.position();

        parent.set_position(base_position + current_position);
        auto result = parent.read_string(max_size);
        parent.set_position(original);

        local_increment_position(max_size);

        return result;
    }


    void SubBuffer::local_increment_position(size_t increment)
    {
        auto new_position = current_position + increment;
        auto byte_size = parent.size();

        if (   (new_position > (byte_size - base_position))
            && (byte_size != -1))
        {
            std::stringstream stream;

            stream << "ByteBuffer position " << new_position << " out of range, " << byte_size
                   << ".";

            throw std::runtime_error(stream.str());
        }

        current_position = new_position;
    }

}

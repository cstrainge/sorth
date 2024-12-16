
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void check_buffer_index(InterpreterPtr& interpreter,
                                const ByteBufferPtr& buffer,
                                int64_t byte_size)
        {
            if ((buffer->position() + byte_size) > buffer->size())
            {
                std::stringstream stream;

                stream << "Writing a value of size " << byte_size << " at a position of "
                       << buffer->position() << " would exceed the buffer size, "
                       << buffer->size() << ".";

                throw_error(interpreter, stream.str());
            }
        }


        void word_buffer_new(InterpreterPtr& interpreter)
        {
            auto size = interpreter->pop_as_size();
            auto buffer = std::make_shared<ByteBuffer>(size);

            interpreter->push(buffer);
        }


        void word_buffer_write_int(InterpreterPtr& interpreter)
        {
            auto byte_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();
            auto value = interpreter->pop_as_integer();

            check_buffer_index(interpreter, buffer, byte_size);
            buffer->write_int(byte_size, value);
        }


        void word_buffer_read_int(InterpreterPtr& interpreter)
        {
            auto is_signed = interpreter->pop_as_bool();
            auto byte_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();

            check_buffer_index(interpreter, buffer, byte_size);
            interpreter->push(buffer->read_int(byte_size, is_signed));
        }


        void word_buffer_write_float(InterpreterPtr& interpreter)
        {
            auto byte_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();
            auto value = interpreter->pop_as_float();

            check_buffer_index(interpreter, buffer, byte_size);
            buffer->write_float(byte_size, value);
        }


        void word_buffer_read_float(InterpreterPtr& interpreter)
        {
            auto byte_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();

            check_buffer_index(interpreter, buffer, byte_size);
            interpreter->push(buffer->read_float(byte_size));
        }


        void word_buffer_write_string(InterpreterPtr& interpreter)
        {
            auto max_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();
            auto value = interpreter->pop_as_string();

            check_buffer_index(interpreter, buffer, max_size);
            buffer->write_string(value, max_size);
        }


        void word_buffer_read_string(InterpreterPtr& interpreter)
        {
            auto max_size = interpreter->pop_as_size();
            auto buffer = interpreter->pop_as_byte_buffer();

            check_buffer_index(interpreter, buffer, max_size);
            interpreter->push(buffer->read_string(max_size));
        }


        void word_buffer_set_position(InterpreterPtr& interpreter)
        {
            auto buffer = interpreter->pop_as_byte_buffer();
            auto new_position = interpreter->pop_as_size();

            buffer->set_position(new_position);
        }


        void word_buffer_get_position(InterpreterPtr& interpreter)
        {
            auto buffer = interpreter->pop_as_byte_buffer();

            interpreter->push(buffer->position());
        }


    }


    void register_buffer_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "buffer.new", word_buffer_new,
            "Create a new byte buffer.",
            "size -- buffer");

        ADD_NATIVE_WORD(interpreter, "buffer.int!", word_buffer_write_int,
            "Write an integer of a given size to the buffer.",
            "value buffer byte_size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.int@", word_buffer_read_int,
            "Read an integer of a given size from the buffer.",
            "buffer byte_size is_signed -- value");

        ADD_NATIVE_WORD(interpreter, "buffer.float!", word_buffer_write_float,
            "Write a float of a given size to the buffer.",
            "value buffer byte_size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.float@", word_buffer_read_float,
            "read a float of a given size from the buffer.",
            "buffer byte_size -- value");

        ADD_NATIVE_WORD(interpreter, "buffer.string!", word_buffer_write_string,
            "Write a string of given size to the buffer.  Padded with 0s if needed.",
            "value buffer size -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.string@", word_buffer_read_string,
            "Read a string of a given max size from the buffer.",
            "buffer size -- value");

        ADD_NATIVE_WORD(interpreter, "buffer.position!", word_buffer_set_position,
            "Set the position of the buffer pointer.",
            "position buffer -- ");

        ADD_NATIVE_WORD(interpreter, "buffer.position@", word_buffer_get_position,
            "Get the position of the buffer pointer.",
            "buffer -- position");
    }


}

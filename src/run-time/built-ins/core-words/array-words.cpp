
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        void throw_if_out_of_bounds(InterpreterPtr& interpreter, int64_t index, int64_t size,
                                    const std::string& type)
        {
            if ((index >= size) || (index < 0))
            {
                std::stringstream stream;

                stream << type << " index, " << index << ", is out of bounds of the size " << size
                       << ".";

                throw_error(interpreter, stream.str());
            }
        }


        void word_array_new(InterpreterPtr& interpreter)
        {
            auto count = interpreter->pop_as_size();
            auto array_ptr = std::make_shared<Array>(count);

            interpreter->push(array_ptr);
        }


        void word_array_size(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();

            interpreter->push(array->size());
        }


        void word_array_write_index(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto index = interpreter->pop_as_size();
            auto new_value = interpreter->pop();

            throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

            (*array)[index] = new_value;
        }


        void word_array_read_index(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto index = interpreter->pop_as_size();

            throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

            interpreter->push((*array)[index]);
        }


        void word_array_insert(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto index = interpreter->pop_as_size();
            auto value = interpreter->pop();

            array->insert(index, value);
        }


        void word_array_delete(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto index = interpreter->pop_as_size();

            throw_if_out_of_bounds(interpreter, index, array->size(), "Array");

            array->remove(index);
        }


        void word_array_resize(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto new_size = interpreter->pop_as_size();

            array->resize(new_size);
        }


        void word_array_plus(InterpreterPtr& interpreter)
        {
            auto array_src = interpreter->pop_as_array();
            auto array_dest = interpreter->pop_as_array();

            auto orig_size = array_dest->size();
            auto new_size = orig_size + array_src->size();

            array_dest->resize(new_size);

            for (auto i = orig_size; i < new_size; ++i)
            {
                (*array_dest)[i] = (*array_src)[i - orig_size].deep_copy();
            }

            interpreter->push(array_dest);
        }



        void word_array_compare(InterpreterPtr& interpreter)
        {
            auto array_a = interpreter->pop_as_array();
            auto array_b = interpreter->pop_as_array();

            interpreter->push(array_a == array_b);
        }


        void word_push_front(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto value = interpreter->pop();

            array->push_front(value);
        }

        void word_push_back(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();
            auto value = interpreter->pop();

            array->push_back(value);
        }

        void word_pop_front(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();

            interpreter->push(array->pop_front(interpreter));
        }

        void word_pop_back(InterpreterPtr& interpreter)
        {
            auto array = interpreter->pop_as_array();

            interpreter->push(array->pop_back(interpreter));
        }


    }


    void register_array_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "[].new", word_array_new,
            "Create a new array with the given default size.",
            "size -- array");

        ADD_NATIVE_WORD(interpreter, "[].size@", word_array_size,
            "Read the size of the array object.",
            "array -- size");

        ADD_NATIVE_WORD(interpreter, "[]!", word_array_write_index,
            "Write to a value in the array.",
            "value index array -- ");

        ADD_NATIVE_WORD(interpreter, "[]@", word_array_read_index,
            "Read a value from the array.",
            "index array -- value");

        ADD_NATIVE_WORD(interpreter, "[].insert", word_array_insert,
            "Grow an array by inserting a value at the given location.",
            "value index array -- ");

        ADD_NATIVE_WORD(interpreter, "[].delete", word_array_delete,
            "Shrink an array by removing the value at the given location.",
            "index array -- ");

        ADD_NATIVE_WORD(interpreter, "[].size!", word_array_resize,
            "Grow or shrink the array to the new size.",
            "new_size array -- ");

        ADD_NATIVE_WORD(interpreter, "[].+", word_array_plus,
            "Take two arrays and deep copy the contents from the second into the "
            "first.",
            "dest source -- dest");

        ADD_NATIVE_WORD(interpreter, "[].=", word_array_compare,
            "Take two arrays and compare the contents to each other.",
            "dest source -- dest");

        ADD_NATIVE_WORD(interpreter, "[].push_front!", word_push_front,
            "Push a value to the front of an array.",
            "value array -- ");

        ADD_NATIVE_WORD(interpreter, "[].push_back!", word_push_back,
            "Push a value to the end of an array.",
            "value array -- ");

        ADD_NATIVE_WORD(interpreter, "[].pop_front!", word_pop_front,
            "Pop a value from the front of an array.",
            "array -- value");

        ADD_NATIVE_WORD(interpreter, "[].pop_back!", word_pop_back,
            "Pop a value from the back of an array.",
            "array -- value");
    }


}


#include "sorth.h"

#ifdef IS_WINDOWS

    #include <windows.h>

#elif defined(IS_UNIX)

    #include <dlfcn.h>

#endif

#include <ffi.h>



namespace sorth
{

    using namespace internal;


    namespace
    {

        #ifdef IS_WINDOWS

            using library_handle = HMODULE;
            using fn_handle = FARPROC;

        #elif defined(IS_UNIX)

            using library_handle = void*;
            using fn_handle = void*;

        #endif


        std::unordered_map<std::string, library_handle> library_map;


        using CalculatedSize = std::tuple<size_t, size_t>;


        using ConversionFrom = std::function<void(InterpreterPtr&, const Value&, Buffer&, Buffer&)>;
        using ConversionTo = std::function<Value(InterpreterPtr&, Buffer&)>;
        using ConversionSize = std::function<CalculatedSize(InterpreterPtr&, const Value&)>;
        using BaseSize = std::function<size_t()>;


        struct ConversionInfo
        {
            ffi_type* type;

            ConversionFrom convert_from;
            ConversionTo convert_to;
            ConversionSize calculate_size;
            BaseSize base_size;
        };


        using ConversionTypeMap = std::unordered_map<std::string, ConversionInfo>;


        ConversionTypeMap create_type_map();


        ConversionTypeMap type_map = std::move(create_type_map());


        struct FfiParamInfo
        {
            ffi_type* type;
            std::string type_name;
            const ConversionInfo& conversion;
        };


        using FfiParams = std::vector<FfiParamInfo>;


        using StructTypeMap = std::unordered_map<std::string, ffi_type>;


        StructTypeMap struct_map;


        ConversionTypeMap create_type_map()
        {
            return
            {
                {
                    "ffi.void",
                    {
                        .type = &ffi_type_void,
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { 0, 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return 0;
                            }
                    }
                },
                {
                    "ffi.i8",
                    {
                        .type = &ffi_type_sint8,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(int8_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int8_t), true);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(int8_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(int8_t);
                            }
                    }
                },
                {
                    "ffi.u8",
                    {
                        .type = &ffi_type_uint8,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(uint8_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint8_t), false);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(uint8_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(uint8_t);
                            }
                    }
                },
                {
                    "ffi.i16",
                    {
                        .type = &ffi_type_sint16,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(int16_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int16_t), true);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(int16_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(uint16_t);
                            }
                    }
                },
                {
                    "ffi.u16",
                    {
                        .type = &ffi_type_uint16,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(uint16_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint16_t), false);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(uint16_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(uint16_t);
                            }
                    }
                },
                {
                    "ffi.i32",
                    {
                        .type = &ffi_type_sint32,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(int32_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int32_t), true);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(int32_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(int32_t);
                            }
                    }
                },
                {
                    "ffi.u32",
                    {
                        .type = &ffi_type_uint32,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(uint32_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint32_t), false);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(uint32_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(uint32_t);
                            }
                    }
                },
                {
                    "ffi.i64",
                    {
                        .type = &ffi_type_sint64,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(int64_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int64_t), true);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(int64_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(int64_t);
                            }
                    }
                },
                {
                    "ffi.u64",
                    {
                        .type = &ffi_type_uint64,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(uint64_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint64_t), false);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(uint64_t), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(uint64_t);
                            }
                    }
                },
                {
                    "ffi.f32",
                    {
                        .type = &ffi_type_float,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_float(sizeof(float),
                                                   as_numeric<double>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                double value = buffer.read_float(sizeof(float));
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(float), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(float);
                            }
                    }
                },
                {
                    "ffi.f64",
                    {
                        .type = &ffi_type_double,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_float(sizeof(double),
                                                   as_numeric<double>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                double value = buffer.read_float(sizeof(double));
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(double), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(double);
                            }
                    }
                },
                {
                    "ffi.string",
                    {
                        .type = &ffi_type_pointer,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                auto string = as_string(interpreter, value);

                                buffer.write_int(sizeof(char*), (size_t)extra.position_ptr());
                                extra.write_string(string, string.size() + 1);
                            },
                        .convert_to = [](auto interpreter, auto& buffer) -> Value
                            {
                                std::string str = reinterpret_cast<char*>(buffer.position_ptr());
                                return str;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                auto string = as_string(interpreter, value);
                                return { sizeof(char*), string.size() + 1 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(char*);
                            }
                    }
                },
                {
                    "ffi.void-ptr",
                    {
                        .type = &ffi_type_pointer,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer, auto& extra)
                            {
                                buffer.write_int(sizeof(uint32_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint32_t), false);
                                return value;
                            },
                        .calculate_size = [](auto interpreter, auto value) -> CalculatedSize
                            {
                                return { sizeof(void*), 0 };
                            },
                        .base_size = []() -> size_t
                            {
                                return sizeof(void*);
                            }
                    }
                }
            };
        }


        const ConversionInfo& get_conversion_info(InterpreterPtr& interpreter,
                                                  const std::string& name)
        {
            auto found = type_map.find(name);

            if (found == type_map.end())
            {
                throw_error(*interpreter, "FFI type " + name + " not found.");
            }

            return found->second;
        }


        std::tuple<FfiParams, std::vector<ffi_type*>>
                         create_param_info(InterpreterPtr& interpreter, const ArrayPtr& param_names)
        {
            FfiParams params;
            std::vector<ffi_type*> types;

            for (int i = 0; i < param_names->size(); ++i)
            {
                auto name = as_string(interpreter, (*param_names)[i]);
                auto& conversion = get_conversion_info(interpreter, name);

                params.push_back(FfiParamInfo
                    {
                        .type = conversion.type,
                        .type_name = name,
                        .conversion = conversion
                    });

                types.push_back(conversion.type);
            }

            return { params, types };
        }


        std::string generate_signature(const FfiParams& params, const std::string& ret_name)
        {
            std::string output = "";

            for (const auto& param : params)
            {
                output += param.type_name + " ";
            }

            if (output.size() == 0)
            {
                output += " ";
            }

            output += "-- ";

            if (ret_name != "ffi.void")
            {
                output += ret_name;
            }

            return output;
        }


        std::vector<void*> pop_params(InterpreterPtr& interpreter,
                                      const FfiParams& params,
                                      ByteBuffer& buffer,
                                      ByteBuffer& ext_buffer)
        {
            std::vector<Value> values;
            std::vector<void*> param_values;

            values.resize(params.size());

            size_t buffer_size = 0;
            size_t ext_buffer_size = 0;

            for (int i = 0; i < params.size(); ++i)
            {
                auto value = interpreter->pop();
                values[params.size() - 1 - i] = value;

                const auto& param = params[params.size() - 1 - i];
                auto [ new_size, new_ext_size ] = param.conversion.calculate_size(interpreter,
                                                                                  value);
                buffer_size += new_size;
                ext_buffer_size += new_ext_size;
            }

            buffer.resize(buffer_size);
            ext_buffer.resize(ext_buffer_size);

            for (int i = 0; i < params.size(); ++i)
            {
                const auto& param = params[i];

                auto& value = values[i];
                void* value_ptr = buffer.position_ptr();

                param.conversion.convert_from(interpreter, value, buffer, ext_buffer);
                param_values.push_back(value_ptr);
            }

            return param_values;
        }


        void push_result(InterpreterPtr&interpreter, const ConversionInfo& conversion,
                         ByteBuffer& buffer)
        {
            auto value = conversion.convert_to(interpreter, buffer);
            interpreter->push(value);
        }


        library_handle load_library(InterpreterPtr& interpreter, const std::string& lib_name)
        {
            library_handle handle =

            #ifdef IS_WINDOWS
                    LoadLibraryA(lib_name.c_str());

                if (handle == nullptr)
                {
                    throw_windows_error(*interpreter,
                                        "Could not load libary " + lib_name + ": ",
                                        GetLastError());
                }
            #elif defined(IS_UNIX)
                    dlopen(lib_name.c_str(), RTLD_NOW);

                if (handle == nullptr)
                {
                    throw_error(*interpreter,
                                std::string("Failed to load external library: ") + dlerror() + ".");
                }
            #endif

            return handle;
        }


        void unload_library(library_handle handle)
        {
            #ifdef IS_WINDOWS
                FreeLibrary(handle);
            #elif defined(IS_UNIX)
                dlclose(handle);
            #endif
        }


        fn_handle find_function(InterpreterPtr& interpreter,
                                const std::string& lib_name,
                                const std::string& function_name)
        {
            auto found = library_map.find(lib_name);

            if (found == library_map.end())
            {
                throw_error(*interpreter, "Failed find open libary: " + lib_name + ".");
            }

            fn_handle fn_handle =
                #ifdef IS_WINDOWS
                    GetProcAddress(found->second, function_name.c_str());

                if (fn_handle == nullptr)
                {
                    throw_windows_error(*interpreter, "Could not find function " + function_name +
                                                      " in library " + lib_name + ": ",
                                        GetLastError());
                }
                #elif defined(IS_UNIX)
                    dlsym(found->second, function_name.c_str());

                if (fn_handle == nullptr)
                {
                    throw_error(*interpreter, "Could not find function " + function_name +
                                              " in library " + lib_name + ".");
                }
                #endif

            return fn_handle;
        }


        void word_ffi_open(InterpreterPtr& interpreter)
        {
            auto register_name = as_string(interpreter, interpreter->pop());
            auto lib_name = as_string(interpreter, interpreter->pop());

            auto handle = load_library(interpreter, lib_name);
            library_map[register_name] = handle;
        }


        void word_ffi_fn(InterpreterPtr& interpreter)
        {
            auto ret_name = as_string(interpreter, interpreter->pop());
            auto raw_params = as_array(interpreter, interpreter->pop());
            auto fn_alias = as_string(interpreter, interpreter->pop());
            auto fn_name = as_string(interpreter, interpreter->pop());
            auto lib_name = as_string(interpreter, interpreter->pop());

            auto function = find_function(interpreter, lib_name, fn_name);
            auto cif = std::make_shared<ffi_cif>();

            auto [ params, param_types ] = create_param_info(interpreter, raw_params);

            auto signature = generate_signature(params, ret_name);
            auto ret_conversion = get_conversion_info(interpreter, ret_name);

            auto word_handler = [=](InterpreterPtr& interpreter)
                {
                    auto types = param_types;

                    if (ffi_prep_cif(cif.get(),
                                     FFI_DEFAULT_ABI,
                                     types.size(),
                                     ret_conversion.type,
                                     types.data()) != FFI_OK)
                    {
                        throw_error(*interpreter, "Could not bind to function " + fn_name + ".");
                    }

                    auto param_buffer = ByteBuffer(0);
                    auto ext_buffer = ByteBuffer(0);
                    auto param_values = pop_params(interpreter, params, param_buffer, ext_buffer);
                    auto ret_size = ret_conversion.base_size();
                    auto ret_buffer = ByteBuffer(ret_size);

                    ffi_call(cif.get(),
                             FFI_FN(function),
                             ret_buffer.data_ptr(),
                             param_values.data());

                    push_result(interpreter, ret_conversion, ret_buffer);
                };

            std::string name = fn_alias != "" ? fn_alias : fn_name;

            ADD_NATIVE_WORD(interpreter,
                            name,
                            word_handler,
                            "Call external function " + fn_name + ".",
                            signature);
        }


        void word_ffi_struct(InterpreterPtr& interpreter)
        {
            auto location = interpreter->get_current_location();

            auto found_initializers = as_numeric<bool>(interpreter, interpreter->pop());
            auto is_hidden = as_numeric<bool>(interpreter, interpreter->pop());
            auto types = as_array(interpreter, interpreter->pop());
            auto fields = as_array(interpreter, interpreter->pop());
            auto packing = as_numeric<int64_t>(interpreter, interpreter->pop());
            auto name = as_string(interpreter, interpreter->pop());

            ArrayPtr defaults;

            if (found_initializers)
            {
                defaults = as_array(interpreter, interpreter->pop());
            }


            // Create the definition object.
            auto definition_ptr = create_data_definition(interpreter,
                                                         name,
                                                         fields,
                                                         defaults,
                                                         is_hidden);

            // Gather conversions for the structure's fields.
            std::vector<ConversionInfo> field_conversions;
            field_conversions.resize(types->size());

            for (size_t i = 0; i < types->size(); ++i)
            {
                field_conversions[i] = get_conversion_info(interpreter,
                                                           as_string(interpreter, (*types)[i]));
            }

            // Centralize the size calculation for the struct.
            auto size_calculation = [=](InterpreterPtr interpreter, Value value) -> CalculatedSize
                {
                    auto data_object = as_data_object(interpreter, value);

                    size_t size = 0;
                    size_t extra = 0;

                    for (int i = 0; i < field_conversions.size(); ++i)
                    {
                        auto [ new_size, new_extra ] =
                                            field_conversions[i].calculate_size(interpreter,
                                                                            data_object->fields[i]);

                        size += new_size;
                        extra += new_extra;
                    }

                    return { size, extra };
                };

            // Create and register the FFI struct information for this data object.
            ffi_type** field_types = new ffi_type*[field_conversions.size() + 1];

            for (size_t i = 0; i < field_conversions.size(); ++i)
            {
                field_types[i] = field_conversions[i].type;
            }

            field_types[field_conversions.size()] = nullptr;

            ffi_type struct_type =
                {
                    .size = 0,
                    .alignment = static_cast<unsigned short>(packing),

                    #ifdef IS_WINDOWS
                        .type = FFI_TYPE_MS_STRUCT,
                    #else
                        .type = FFI_TYPE_STRUCT,
                    #endif

                    .elements = field_types,
                };

            auto iter = struct_map.find(name);

            if (iter != struct_map.end())
            {
                delete [] iter->second.elements;
            }

            struct_map[name] = struct_type;

            // Create a converter for the struct.
            auto base_conversion = ConversionInfo
                {
                    .type = &struct_map[name],
                    .convert_from = [=](auto interpreter, auto& value, auto& buffer, auto& extra)
                        {
                            auto data_object = as_data_object(interpreter, value);

                            for (int i = 0; i < field_conversions.size(); ++i)
                            {
                                field_conversions[i].convert_from(interpreter,
                                                                  data_object->fields[i],
                                                                  buffer,
                                                                  extra);
                            }
                        },
                    .convert_to = [=](auto interpreter, auto& buffer) -> Value
                        {
                            auto data = make_data_object(interpreter, definition_ptr);

                            for (int i = 0; i < field_conversions.size(); ++i)
                            {
                                auto value = field_conversions[i].convert_to(interpreter, buffer);
                                data->fields[i] = value;
                            }

                            return data;
                        },
                    .calculate_size = [=](auto interpreter, auto value) -> CalculatedSize
                        {
                            return size_calculation(interpreter, value);
                        },
                    .base_size = [=]() -> size_t
                        {
                            size_t size = 0;

                            for (int i = 0; i < field_conversions.size(); ++i)
                            {
                                auto new_size = field_conversions[i].base_size();

                                size += new_size;
                            }

                            return size;
                        }
                };

            // Create a converter for a struct pointer.
            auto ptr_conversion = ConversionInfo
                {
                    .type = &ffi_type_pointer,
                    .convert_from = [=](auto interpreter, auto& value, auto& buffer, auto& extra)
                        {
                            auto [ size, extra_size ] = size_calculation(interpreter, value);

                            auto data_object = as_data_object(interpreter, value);
                            auto sub_buffer = SubBuffer(extra, extra_size);

                            buffer.write_int(sizeof(char*), (size_t)extra.position_ptr());

                            for (int i = 0; i < field_conversions.size(); ++i)
                            {
                                field_conversions[i].convert_from(interpreter,
                                                                  data_object->fields[i],
                                                                  extra,
                                                                  sub_buffer);
                            }
                        },
                    .convert_to = [=](auto interpreter, auto& buffer) -> Value
                        {
                            auto value = buffer.read_int(sizeof(void*), false);
                            void* raw_ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(value));
                            auto raw_buffer = ByteBuffer(raw_ptr, -1);
                            auto data = make_data_object(interpreter, definition_ptr);

                            for (int i = 0; i < field_conversions.size(); ++i)
                            {
                                auto value = field_conversions[i].convert_to(interpreter,
                                                                             raw_buffer);
                                data->fields[i] = value;
                            }

                            return data;
                        },
                    .calculate_size = [=](auto interpreter, auto value) -> CalculatedSize
                        {
                            auto [ size, extra ] = size_calculation(interpreter, value);
                            return { sizeof(void*), size + extra };
                        },
                    .base_size = []() -> size_t
                        {
                            return sizeof(void*);
                        }
                };

            // Register our converters with the type map.
            type_map[name] = base_conversion;
            type_map[name + "-ptr"] = ptr_conversion;

            // Create the words to allow the script to access this definition.  The word
            // <definition_name>.new will always hold a base reference to our definition object.
            create_data_definition_words(location, interpreter, definition_ptr, is_hidden);
        }

    }


    void register_ffi_words(InterpreterPtr& interpreter)
    {
        ADD_NATIVE_WORD(interpreter, "ffi.load", word_ffi_open,
            "Load an binary library and register it with the ffi interface.",
            "lib-name -- ");

        ADD_NATIVE_WORD(interpreter, "ffi.fn", word_ffi_fn,
            "Bind to an external function.",
            "lib-name fn-name fn-alias fn-params ret-name -- ");

        ADD_NATIVE_WORD(interpreter, "ffi.#", word_ffi_struct,
            "Create a strucure compatable with the ffi interface.",
            "lib-name -- ");

        ADD_NATIVE_WORD(interpreter, "ffi.[]", word_ffi_struct,
            "Register a new ffi array type for the specified ff.",
            "struct-name -- ");
    }


    void reset_ffi()
    {
        type_map = std::move(create_type_map());

        for (auto& item : struct_map)
        {
            delete [] item.second.elements;
        }

        struct_map.clear();

        for (auto& item : library_map)
        {
            unload_library(item.second);
        }

        library_map.clear();
    }

}

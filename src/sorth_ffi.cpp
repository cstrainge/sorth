
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


        struct ConversionInfo
        {
            int64_t size;
            ffi_type* type;
            bool is_ptr;

            std::function<void(InterpreterPtr&, const Value&, ByteBuffer&)> convert_from;
            std::function<Value(InterpreterPtr&, ByteBuffer&)> convert_to;

            std::function<size_t(InterpreterPtr&, const Value&)> calculate_size;
        };


        std::unordered_map<std::string, ConversionInfo> type_map =
            {
                {
                    "ffi.void",
                    {
                        .size = 0,
                        .type = &ffi_type_void,
                        .is_ptr = false
                    }
                },
                {
                    "ffi.i8",
                    {
                        .size = sizeof(int8_t),
                        .type = &ffi_type_sint8,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(int8_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int8_t), true);
                                return value;
                            }
                    }
                },
                {
                    "ffi.u8",
                    {
                        .size = sizeof(uint8_t),
                        .type = &ffi_type_uint8,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(uint8_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint8_t), false);
                                return value;
                            }
                    }
                },
                {
                    "ffi.i16",
                    {
                        .size = sizeof(int16_t),
                        .type = &ffi_type_sint16,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(int16_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int16_t), true);
                                return value;
                            }
                    }
                },
                {
                    "ffi.u16",
                    {
                        .size = sizeof(uint16_t),
                        .type = &ffi_type_uint16,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(uint16_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint16_t), false);
                                return value;
                            }
                    }
                },
                {
                    "ffi.i32",
                    {
                        .size = sizeof(int32_t),
                        .type = &ffi_type_sint32,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(int32_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int32_t), true);
                                return value;
                            }
                    }
                },
                {
                    "ffi.u32",
                    {
                        .size = sizeof(uint32_t),
                        .type = &ffi_type_uint32,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(uint32_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint32_t), false);
                                return value;
                            }
                    }
                },
                {
                    "ffi.i64",
                    {
                        .size = sizeof(int64_t),
                        .type = &ffi_type_sint64,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(int64_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(int64_t), true);
                                return value;
                            }
                    }
                },
                {
                    "ffi.u64",
                    {
                        .size = sizeof(uint64_t),
                        .type = &ffi_type_uint64,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_int(sizeof(uint64_t),
                                                 as_numeric<int64_t>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                int64_t value = buffer.read_int(sizeof(uint64_t), false);
                                return value;
                            }
                    }
                },
                {
                    "ffi.f32",
                    {
                        .size = sizeof(float),
                        .type = &ffi_type_float,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_float(sizeof(float),
                                                   as_numeric<double>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                double value = buffer.read_float(sizeof(float));
                                return value;
                            }
                    }
                },
                {
                    "ffi.f64",
                    {
                        .size = sizeof(double),
                        .type = &ffi_type_double,
                        .is_ptr = false,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                buffer.write_float(sizeof(double),
                                                   as_numeric<double>(interpreter, value));
                            },
                        .convert_to = [](auto Interpreter, auto& buffer) -> Value
                            {
                                double value = buffer.read_float(sizeof(double));
                                return value;
                            }
                    }
                },
                {
                    "ffi.string",
                    {
                        .size = -1,
                        .type = &ffi_type_pointer,
                        .is_ptr = true,
                        .convert_from = [](auto interpreter, auto& value, auto& buffer)
                            {
                                auto string = as_string(interpreter, value);
                                buffer.write_string(string, string.size() + 1);
                            },
                        .convert_to = [](auto interpreter, auto& buffer) -> Value
                            {
                                std::string str = reinterpret_cast<char*>(buffer.position_ptr());
                                return str;
                            },
                        .calculate_size = [](auto interpreter, auto value)
                            {
                                auto string = as_string(interpreter, value);
                                return string.size() + 1 + sizeof(void*);
                            }
                    }
                }
            };


        struct FfiParamInfo
        {
            ffi_type* type;
            std::string& type_name;
            const ConversionInfo& conversion;
        };


        using FfiParams = std::vector<FfiParamInfo>;


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
                                      ByteBuffer& buffer)
        {
            std::vector<Value> values;
            std::vector<void*> param_values;

            values.resize(params.size());

            size_t buffer_size = 0;

            for (int i = 0; i < params.size(); ++i)
            {
                auto value = interpreter->pop();
                values[params.size() - 1 - i] = value;

                const auto& param = params[params.size() - 1 - i];
                size_t size = param.conversion.size != -1
                              ? param.conversion.size
                              : param.conversion.calculate_size(interpreter, value) + sizeof(void*);
                buffer_size += size;
            }

            buffer.resize(buffer_size);

            for (int i = 0; i < params.size(); ++i)
            {
                const auto& param = params[i];

                auto& value = values[i];

                void* value_ptr = buffer.position_ptr();

                if (param.conversion.is_ptr)
                {
                    auto start_pos = buffer.position();

                    buffer.set_position(start_pos + sizeof(void*));
                    auto data_ptr = buffer.position_ptr();

                    param.conversion.convert_from(interpreter, value, buffer);

                    auto end_pos = buffer.position();
                    buffer.set_position(start_pos);
                    buffer.write_int(sizeof(void*), (size_t)data_ptr);

                    buffer.set_position(end_pos);
                }
                else
                {
                    param.conversion.convert_from(interpreter, value, buffer);
                }

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


        void word_ffi_open(InterpreterPtr& Interpreter)
        {
            auto register_name = as_string(Interpreter, Interpreter->pop());
            auto lib_name = as_string(Interpreter, Interpreter->pop());

            auto handle = load_library(Interpreter, lib_name);
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

            auto word_handler = [cif, fn_name, function, params, param_types, ret_conversion](InterpreterPtr& interpreter)
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
                    auto param_values = pop_params(interpreter, params, param_buffer);
                    auto result = ByteBuffer(ret_conversion.size != -1
                                             ? ret_conversion.size : sizeof(void*));

                    ffi_call(cif.get(), FFI_FN(function), result.data_ptr(), param_values.data());
                    push_result(interpreter, ret_conversion, result);
                };

            std::string name = fn_alias != "" ? fn_alias : fn_name;

            ADD_NATIVE_WORD(interpreter,
                            name,
                            word_handler,
                            "Call external function " + fn_name + ".",
                            signature);
        }

    }


    void register_ffi_words(InterpreterPtr& interpreter)
    {
        // TODO: Allow for structures: #.ffi ... ;

        ADD_NATIVE_WORD(interpreter, "ffi.load", word_ffi_open,
            "Load an binary library and register it with the ffi interface.",
            "lib-name -- ");

        ADD_NATIVE_WORD(interpreter, "ffi.fn", word_ffi_fn,
            "Bind to an external function.",
            "lib-name fn-name fn-alias fn-params ret-name -- ");
    }

}


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

            using library_handle = HANDLE;
            using fn_handle = FARPROC;

        #elif defined(IS_UNIX)

            using library_handle = void*;
            using fn_handle = void*;

        #endif


        std::unordered_map<std::string, library_handle> library_map;


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
                #elif defined(IS_UNIX)
                    dlsym(found->second, function_name.c_str());
                #endif

            if (fn_handle == nullptr)
            {
                throw_error(*interpreter, "Could not find function" + function_name +
                                          " in library " + lib_name + ".");
            }

            return fn_handle;
        }


        struct ConversionInfo
        {
            size_t size;
            ffi_type* type;
            bool is_ptr;

            std::function<void(InterpreterPtr&, const Value&, void*)> convert_from;
            std::function<Value(InterpreterPtr&, void*)> convert_to;
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(int8_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<int8_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(uint8_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<uint8_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(int16_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<int16_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(uint16_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<uint16_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(int32_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<int32_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(uint32_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<uint32_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(int64_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<int64_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(uint64_t*)buffer = as_numeric<int64_t>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                int64_t value = *reinterpret_cast<uint64_t*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(float*)buffer = as_numeric<double>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                double value = *reinterpret_cast<float*>(buffer);
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
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                *(double*)buffer = as_numeric<double>(interpreter, value);
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                double value = *reinterpret_cast<double*>(buffer);
                                return value;
                            }
                    }
                },
                {
                    "ffi.string",
                    {
                        .size = sizeof(char*),
                        .type = &ffi_type_pointer,
                        .is_ptr = true,
                        .convert_from = [](auto interpreter, auto value, auto buffer)
                            {
                                auto string = as_string(interpreter, value);
                                auto str_buffer = new char[string.size() + 1];

                                memcpy(str_buffer, string.c_str(), string.size());
                                ((char*)str_buffer)[string.size()] = 0;

                                *(reinterpret_cast<char**>(buffer)) = str_buffer;
                            },
                        .convert_to = [](auto Interpreter, auto buffer) -> Value
                            {
                                std::string str = reinterpret_cast<char*>(buffer);
                                return str;
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


        ffi_type* get_type_from_name(InterpreterPtr& interpreter, const std::string& name)
        {
            return get_conversion_info(interpreter, name).type;
        }


        std::tuple<FfiParams,
                   std::vector<ffi_type*>> create_param_info(InterpreterPtr& interpreter,
                                                             const ArrayPtr& param_names)
        {
            FfiParams params;
            std::vector<ffi_type*> types;

            for (int i = 0; i < param_names->size(); ++i)
            {
                auto name = as_string(interpreter, (*param_names)[i]);
                auto& conversion = get_conversion_info(interpreter, name);

                params.push_back(FfiParamInfo {
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

            //for (const auto& param : params)
            for (auto it = params.rbegin(); it != params.rend(); ++it)
            {
                const auto& param = *it;
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


        void free_params(const FfiParams& params, std::vector<void*>& param_values)
        {
            for (int i = 0; i < param_values.size(); ++i)
            {
                if (params[i].conversion.is_ptr)
                {
                    void* inner_pointer = *reinterpret_cast<void**>(param_values[i]);
                    delete [] static_cast<char*>(inner_pointer);
                }

                delete[] static_cast<char*>(param_values[i]);
            }
        }


        void* allocate_type(InterpreterPtr& interpreter, const std::string& ret_name)
        {
            const auto& conversion = get_conversion_info(interpreter, ret_name);
            return new char[conversion.size];
        }


        std::vector<void*> pop_params(InterpreterPtr& interpreter, const FfiParams& params)
        {
            std::vector<void*> param_values;

            try
            {
                //for (const auto& param : params)
                for (auto it = params.rbegin(); it != params.rend(); ++it)
                {
                    const auto& param = *it;

                    auto value = interpreter->pop();
                    size_t size = param.conversion.size;
                    void* buffer = new char[size];

                    param.conversion.convert_from(interpreter, value, buffer);
                    param_values.insert(param_values.begin(), buffer);
                }
            }
            catch (...)
            {
                free_params(params, param_values);
                throw;
            }

            return param_values;
        }


        void push_result(InterpreterPtr& interpreter,
                         const std::string& ret_name,
                         void* result)
        {
            const auto& conversion = get_conversion_info(interpreter, ret_name);
            interpreter->push(conversion.convert_to(interpreter, result));
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

            if (ffi_prep_cif(cif.get(),
                             FFI_DEFAULT_ABI,
                             param_types.size(),
                             get_type_from_name(interpreter, ret_name),
                             param_types.data()) != FFI_OK)
            {
                throw_error(*interpreter, "Could not bind to function " + fn_name + ".");
            }

            auto word_handler = [cif, function, params, ret_name](InterpreterPtr& interpreter)
                {
                    auto param_values = pop_params(interpreter, params);
                    void* result = allocate_type(interpreter, ret_name);

                    ffi_call(cif.get(), FFI_FN(function), result, param_values.data());
                    push_result(interpreter, ret_name, result);

                    free_params(params, param_values);

                    delete[] static_cast<char*>(result);
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

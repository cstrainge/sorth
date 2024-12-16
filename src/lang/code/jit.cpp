
#include "sorth.h"



// Make sure we found llvm.
#ifndef SORTH_JIT_DISABLED

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/Object/SymbolicFile.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>



namespace sorth::internal
{


    namespace
    {


        // We override the default memory manager for the JIT engine so that we can track the size
        // of the functions that are JITed.  This is so that we can later disassemble the JITed
        // code.
        class TrackingMemoryManager : public llvm::SectionMemoryManager
        {
            public:
                static std::unordered_map<std::string, std::tuple<uint64_t, size_t>> FunctionSizes;

            public:
                virtual void notifyObjectLoaded(llvm::RuntimeDyld& rt_dyld,
                                                const llvm::object::ObjectFile& obj) override
                {
                    SectionMemoryManager::notifyObjectLoaded(rt_dyld, obj);

                    // Gather information about the functions in the object file.
                    for (auto& symbol : obj.symbols())
                    {
                        auto expected_type = symbol.getType();

                        // If the symbol is a function, then we'll gather information about it.
                        if (   (expected_type)
                            && (expected_type.get() == llvm::object::SymbolRef::ST_Function))
                        {
                            // Gether information about the function.
                            auto name = symbol.getName().get().str();
                            auto address = rt_dyld.getSymbolLocalAddress(name);
                            auto size = symbol.getCommonSize();

                            // The method getCommonSize can't always return a size for functions.
                            // If it returns 0, then we'll try to calculate the size by looking at
                            // the next symbol in the same section.  Not a 100% reliable method, but
                            // it's better than nothing.
                            if (size == 0)
                            {
                                size = try_calculate_size(obj, symbol);
                            }

                            // Save the function's address and size. If we could find them.
                            if ((address) && (size > 0))
                            {
                                FunctionSizes.insert({ name, { (uint64_t)address, size } });
                            }
                        }
                    }
                }

            private:
                // If we're on a platform that doesn't provide the size of a function in the symbol
                // table, we can try to calculate it by looking at the next symbol in the same
                // section.
                size_t try_calculate_size(const llvm::object::ObjectFile& obj,
                                        const llvm::object::SymbolRef& symbol)
                {
                    // Get the section and address of the symbol.
                    auto expected_section = symbol.getSection();
                    if (!expected_section)
                    {
                        return 0;
                    }

                    auto section = expected_section.get();

                    // Get the address of the symbol.
                    auto expected_address = symbol.getAddress();
                    if (!expected_address)
                    {
                        return 0;
                    }

                    uint64_t address = expected_address.get();

                    // Try to find the next symbol in the same section.
                    uint64_t next_symbol_address = ~0;

                    for (const auto& next_symbol : obj.symbols())
                    {
                        // Get the section of the next symbol.
                        auto expected_next_section = next_symbol.getSection();

                        // Is the symbol in the same section as ours?
                        if (   (expected_next_section)
                            && (expected_next_section.get() == section))
                        {
                            // The symbol is in the same section, so get it's address.
                            auto expected_next_address = next_symbol.getAddress();

                            if (   (expected_next_address)
                                && (expected_next_address.get() > address))
                            {
                                // Now, check if this symbol is closer to our symbol than the last
                                // one we found.
                                auto try_next_address = expected_next_address.get();

                                if (   (try_next_address > address)
                                    && (try_next_address < next_symbol_address))
                                {
                                    // This symbol is closer to ours than the last one we found.
                                    // So use it as the next symbol address until we find a closer
                                    // one.
                                    next_symbol_address = try_next_address;
                                }
                            }
                        }
                    }

                    // If we found a next symbol address, then we can calculate the size of our
                    // symbol.
                    if (next_symbol_address != ~0)
                    {
                        return next_symbol_address - address;
                    }

                    // If we didn't find a next symbol in the same section, then we can calculate
                    // the size of our symbol by subtracting it's address from the end of the
                    // section.
                    return section->getSize() - (address - section->getAddress());
                }
        };


        // Keep track of the size of the functions that are JITed.
        std::unordered_map<std::string, std::tuple<uint64_t, size_t>>
                                                               TrackingMemoryManager::FunctionSizes;


        // On macOS symbols must be prefixed by an underscore.  We'll use this constant to handle
        // that later on in the code.
        #if defined(IS_MACOS)
            const std::string platform_prefix = "_";
        #else
            const std::string platform_prefix = "";
        #endif


        // What type of code generation are we doing?
        enum class CodeGenType
        {
            // We are generating a word.
            word,

            // We are generating the top level of a script.
            script_body
        };


        // The JIT engine, we hold the llvm context here.
        struct JitEngine
        {
            // The llvm execution engine used for JITing code.
            std::unique_ptr<llvm::orc::LLJIT> jit = nullptr;

            // Function type info pointers for the JITed code helper functions.
            llvm::Function* handle_set_location_fn = nullptr;
            llvm::Function* handle_manage_context_fn = nullptr;

            llvm::Function* handle_define_variable_fn = nullptr;
            llvm::Function* handle_define_constant_fn = nullptr;
            llvm::Function* handle_read_variable_fn = nullptr;
            llvm::Function* handle_write_variable_fn = nullptr;

            llvm::Function* handle_pop_bool_fn = nullptr;

            llvm::Function* handle_push_last_exception_fn = nullptr;
            llvm::Function* handle_push_bool_fn = nullptr;
            llvm::Function* handle_push_int_fn = nullptr;
            llvm::Function* handle_push_double_fn = nullptr;
            llvm::Function* handle_push_string_fn = nullptr;
            llvm::Function* handle_push_value_fn = nullptr;

            llvm::Function* handle_word_execute_name_fn = nullptr;
            llvm::Function* handle_word_execute_index_fn = nullptr;

            llvm::Function* handle_word_index_name_fn = nullptr;
            llvm::Function* handle_word_exists_name_fn = nullptr;

            // For storing any errors that occur in the llvm engine.
            std::string llvm_error_str;

            // Keep track of the last exception that occurred in the JITed code for each thread.
            static thread_local std::optional<std::runtime_error> last_exception;


            // Constructor for the JIT engine.  Initialize LLVM and create the execution engine.
            // Also register our helper functions that will get called from the JITed code.
            JitEngine()
            {
                // Initialize the llvm jit engine.
                llvm::InitializeNativeTarget();
                llvm::InitializeNativeTargetAsmPrinter();
                llvm::InitializeNativeTargetAsmParser();
                llvm::InitializeNativeTargetDisassembler();

                // Construct the LLVM JIT engine, using the object linking layer creator to create
                // and use our custom memory manager.
                auto jit_result =
                    llvm::orc::LLJITBuilder()
                        .setObjectLinkingLayerCreator([=](llvm::orc::ExecutionSession& es,
                                                          const llvm::Triple& triple)
                                          -> llvm::Expected<std::unique_ptr<llvm::orc::ObjectLayer>>
                        {
                            // Create a standard object linking layer with our custom memory
                            // manager, instead of the default one.
                            auto layer = new llvm::orc::RTDyldObjectLinkingLayer(
                                    es,
                                    [=]() -> std::unique_ptr<llvm::RuntimeDyld::MemoryManager>
                                    {
                                        return std::make_unique<TrackingMemoryManager>();
                                    });

                            // Return our new object linking layer.
                            return std::unique_ptr<llvm::orc::ObjectLayer>(layer);
                        })
                        .create();

                if (!jit_result)
                {
                    std::string error_message;

                    llvm::raw_string_ostream stream(error_message);
                    llvm::logAllUnhandledErrors(jit_result.takeError(),
                                                stream,
                                                "Failed to create llvm JIT engine: ");
                    stream.flush();

                    throw_error(error_message);
                }

                // Save the jit engine and register the helper function symbols/types.
                jit = std::move(*jit_result);
                register_jit_helper_ptrs();
            }


            // Map the helper function pointers to their symbols in the JIT engine.
            void register_jit_helper_ptrs()
            {
                // Properly mangle symbols for the platform's ABI, on macOS this means we need a
                // leading underscore.
                auto mangle = [this](const std::string& name) -> llvm::orc::SymbolStringPtr
                    {
                        return jit->getExecutionSession().intern(platform_prefix + name);
                    };

                // Create the symbol map for the helper functions.
                llvm::orc::SymbolMap symbol_map =
                    {
                        {
                            mangle("handle_set_location"),
                            llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_set_location),
                                         llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_manage_context"),
                            llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_manage_context),
                                         llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_define_variable"),
                            llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_define_variable),
                                         llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_define_constant"),
                            llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_define_constant),
                                         llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_read_variable"),
                            llvm::orc::ExecutorSymbolDef(
                                           llvm::orc::ExecutorAddr((uint64_t)&handle_read_variable),
                                           llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_write_variable"),
                            llvm::orc::ExecutorSymbolDef(
                                          llvm::orc::ExecutorAddr((uint64_t)&handle_write_variable),
                                          llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_pop_bool"),
                            llvm::orc::ExecutorSymbolDef(
                                                llvm::orc::ExecutorAddr((uint64_t)&handle_pop_bool),
                                                llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_last_exception"),
                            llvm::orc::ExecutorSymbolDef(
                                     llvm::orc::ExecutorAddr((uint64_t)&handle_push_last_exception),
                                     llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_bool"),
                            llvm::orc::ExecutorSymbolDef(
                                               llvm::orc::ExecutorAddr((uint64_t)&handle_push_bool),
                                               llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_int"),
                            llvm::orc::ExecutorSymbolDef(
                                                llvm::orc::ExecutorAddr((uint64_t)&handle_push_int),
                                                llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_double"),
                            llvm::orc::ExecutorSymbolDef(
                                            llvm::orc::ExecutorAddr((uint64_t)&handle_push_double),
                                            llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_string"),
                            llvm::orc::ExecutorSymbolDef(
                                            llvm::orc::ExecutorAddr((uint64_t)&handle_push_string),
                                            llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_push_value"),
                            llvm::orc::ExecutorSymbolDef(
                                            llvm::orc::ExecutorAddr((uint64_t)&handle_push_value),
                                            llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_word_execute_name"),
                            llvm::orc::ExecutorSymbolDef(
                                       llvm::orc::ExecutorAddr((uint64_t)&handle_word_execute_name),
                                       llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_word_execute_index"),
                            llvm::orc::ExecutorSymbolDef(
                                      llvm::orc::ExecutorAddr((uint64_t)&handle_word_execute_index),
                                      llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_word_index_name"),
                            llvm::orc::ExecutorSymbolDef(
                                         llvm::orc::ExecutorAddr((uint64_t)&handle_word_index_name),
                                         llvm::JITSymbolFlags::Exported)
                        },
                        {
                            mangle("handle_word_exists_name"),
                            llvm::orc::ExecutorSymbolDef(
                                        llvm::orc::ExecutorAddr((uint64_t)&handle_word_exists_name),
                                        llvm::JITSymbolFlags::Exported)
                        }
                    };

                // Register that map with the JIT engine.
                auto error = jit->getMainJITDylib()
                                         .define(llvm::orc::absoluteSymbols(std::move(symbol_map)));

                if (error)
                {
                    std::string error_message;

                    llvm::raw_string_ostream stream(error_message);
                    llvm::logAllUnhandledErrors(std::move(error),
                                                stream,
                                                "Failed to define helper symbols: ");
                    stream.flush();

                    throw_error(error_message);
                }
            }


            // Register the helper functions and their signatures that will be called from the JITed
            // code.
            void register_jit_helpers(std::unique_ptr<llvm::Module>& module,
                                      std::unique_ptr<llvm::LLVMContext>& context)
            {
                // Gather up our basic types needed for the function signatures.
                auto void_type = llvm::Type::getVoidTy(*context.get());
                auto ptr_type = llvm::PointerType::getUnqual(void_type);
                auto int64_type = llvm::Type::getInt64Ty(*context.get());
                auto bool_type = llvm::Type::getInt1Ty(*context.get());
                auto bool_ptr_type = llvm::PointerType::getUnqual(bool_type);
                auto double_type = llvm::Type::getDoubleTy(*context.get());
                auto char_type = llvm::Type::getInt8Ty(*context.get());
                auto char_ptr_type = llvm::PointerType::getUnqual(char_type);


                // Register the handle_set_location function.
                auto handle_set_location_type = llvm::FunctionType::get(void_type,
                                                                 { ptr_type, ptr_type, int64_type },
                                                                 false);
                handle_set_location_fn = llvm::Function::Create(handle_set_location_type,
                                                                llvm::Function::ExternalLinkage,
                                                                "handle_set_location",
                                                                module.get());

                // Register the handle_manage_context function.
                auto handle_manage_context_type = llvm::FunctionType::get(void_type,
                                                                          { ptr_type, bool_type },
                                                                          false);
                handle_manage_context_fn = llvm::Function::Create(handle_manage_context_type,
                                                                  llvm::Function::ExternalLinkage,
                                                                  "handle_manage_context",
                                                                  module.get());

                // Register the handle_define_variable function.
                auto handle_define_variable_type = llvm::FunctionType::get(int64_type,
                                                                        { ptr_type, char_ptr_type },
                                                                        false);
                handle_define_variable_fn = llvm::Function::Create(handle_define_variable_type,
                                                                   llvm::Function::ExternalLinkage,
                                                                   "handle_define_variable",
                                                                   module.get());

                // Register the handle_define_constant function.
                auto handle_define_constant_type = llvm::FunctionType::get(int64_type,
                                                                        { ptr_type, char_ptr_type },
                                                                        false);
                handle_define_constant_fn = llvm::Function::Create(handle_define_constant_type,
                                                                   llvm::Function::ExternalLinkage,
                                                                   "handle_define_constant",
                                                                   module.get());

                // Register the handle_read_variable function.
                auto handle_read_variable_type = llvm::FunctionType::get(int64_type, { ptr_type },
                                                                         false);
                handle_read_variable_fn = llvm::Function::Create(handle_read_variable_type,
                                                                 llvm::Function::ExternalLinkage,
                                                                 "handle_read_variable",
                                                                 module.get());

                // Register the handle_write_variable function.
                auto handle_write_variable_type = llvm::FunctionType::get(int64_type, { ptr_type },
                                                                          false);
                handle_write_variable_fn = llvm::Function::Create(handle_write_variable_type,
                                                                  llvm::Function::ExternalLinkage,
                                                                  "handle_write_variable",
                                                                  module.get());

                // Register the handle_pop_bool function.
                auto handle_pop_bool_type = llvm::FunctionType::get(int64_type,
                                                                    { ptr_type, bool_ptr_type },
                                                                    false);
                handle_pop_bool_fn = llvm::Function::Create(handle_pop_bool_type,
                                                            llvm::Function::ExternalLinkage,
                                                            "handle_pop_bool",
                                                            module.get());

                // Register the handle_push_last_exception function.
                auto handle_push_last_exception_type = llvm::FunctionType::get(void_type,
                                                                        { ptr_type },
                                                                        false);
                handle_push_last_exception_fn =
                                             llvm::Function::Create(handle_push_last_exception_type,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "handle_push_last_exception",
                                                                    module.get());

                // Register the handle_push_bool function.
                auto handle_push_bool_type = llvm::FunctionType::get(void_type,
                                                                     { ptr_type, bool_type },
                                                                     false);
                handle_push_bool_fn = llvm::Function::Create(handle_push_bool_type,
                                                             llvm::Function::ExternalLinkage,
                                                             "handle_push_bool",
                                                             module.get());

                // Register the handle_push_int function.
                auto handle_push_int_type = llvm::FunctionType::get(void_type,
                                                                    { ptr_type, int64_type },
                                                                    false);
                handle_push_int_fn = llvm::Function::Create(handle_push_int_type,
                                                            llvm::Function::ExternalLinkage,
                                                            "handle_push_int",
                                                            module.get());

                // Register the handle_push_double function.
                auto handle_push_double_type = llvm::FunctionType::get(void_type,
                                                                       { ptr_type, double_type },
                                                                       false);
                handle_push_double_fn = llvm::Function::Create(handle_push_double_type,
                                                               llvm::Function::ExternalLinkage,
                                                               "handle_push_double",
                                                               module.get());

                // Register the handle_push_string function.
                auto handle_push_string_type = llvm::FunctionType::get(void_type,
                                                                      { ptr_type, char_ptr_type },
                                                                      false);
                handle_push_string_fn = llvm::Function::Create(handle_push_string_type,
                                                               llvm::Function::ExternalLinkage,
                                                               "handle_push_string",
                                                               module.get());

                // Register the handle_push_value function.
                auto handle_push_value_type = llvm::FunctionType::get(void_type,
                                                                 { ptr_type, ptr_type, int64_type },
                                                                 false);
                handle_push_value_fn = llvm::Function::Create(handle_push_value_type,
                                                              llvm::Function::ExternalLinkage,
                                                              "handle_push_value",
                                                              module.get());

                // Register the handle_word_execute_name function.
                auto handle_word_execute_name_type = llvm::FunctionType::get(int64_type,
                                                                        { ptr_type, char_ptr_type },
                                                                        false);
                handle_word_execute_name_fn = llvm::Function::Create(handle_word_execute_name_type,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "handle_word_execute_name",
                                                                    module.get());

                // Register the handle_word_execute_index function.
                auto handle_word_execute_index_type = llvm::FunctionType::get(int64_type,
                                                                        { ptr_type, int64_type },
                                                                        false);
                handle_word_execute_index_fn = llvm::Function::Create(
                                                                    handle_word_execute_index_type,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "handle_word_execute_index",
                                                                    module.get());

                // Register the handle_word_index_name function.
                auto handle_word_index_name_type = llvm::FunctionType::get(void_type,
                                                                        { ptr_type, char_ptr_type },
                                                                        false);
                handle_word_index_name_fn = llvm::Function::Create(handle_word_index_name_type,
                                                                   llvm::Function::ExternalLinkage,
                                                                   "handle_word_index_name",
                                                                   module.get());

                // Register the handle_word_exists_name function.
                auto handle_word_exists_name_type = llvm::FunctionType::get(void_type,
                                                                        { ptr_type, char_ptr_type },
                                                                        false);
                handle_word_exists_name_fn = llvm::Function::Create(handle_word_exists_name_type,
                                                                    llvm::Function::ExternalLinkage,
                                                                    "handle_word_exists_name",
                                                                    module.get());
            }


            // JIT compile the given byte-code block into a native function handler.
            WordFunction jit_bytecode(InterpreterPtr& interpreter, const Construction& construction)
            {
                // Make sure we have a name for the word that's usable for the JIT engine.
                auto filtered_name = filter_word_name(construction.name);

                // Create the context for this compilation.
                auto [ module, context ] = create_jit_module_context(filtered_name);

                // Jit compile the word and then finalize it's module.
                auto [ locations, constants ] = jit_compile(interpreter,
                                                            module,
                                                            context,
                                                            filtered_name,
                                                            construction.code,
                                                            CodeGenType::word);

                // JIT compile and optimize the module, returning the IR for the word.
                auto ir_map = finalize_module(std::move(module), std::move(context));

                TrackingMemoryManager::FunctionSizes.clear();

                // Finally return the new word handler function.
                return create_word_function(filtered_name,
                                            std::move(ir_map[filtered_name]),
                                            std::move(locations),
                                            std::move(constants),
                                            CodeGenType::word);
            }


            // JIT compile the given byte-code block into the script's top level function handler.
            // As well as all of the script's non-immediate words that have been cached during the
            // byte-code compilation phase.
            WordFunction jit_bytecode(InterpreterPtr& interpreter,
                                      const std::string& name,
                                      const ByteCode& code,
                                      const std::map<std::string, Construction>& word_jit_cache)
            {
                struct GeneratedWord
                {
                    std::string name;
                    std::vector<Location> locations;
                    std::vector<Value> constants;
                };

                std::map<std::string, GeneratedWord> generated_words;

                // Create the context for this compilation.
                auto [ module, context ] = create_jit_module_context(name);

                // JIT compile the scripts words.
                for (const auto& [ word_name, construction ] : word_jit_cache)
                {
                    auto filtered_name = filter_word_name(word_name);

                    auto [ locations, constants ] = jit_compile(interpreter,
                                                                module,
                                                                context,
                                                                filtered_name,
                                                                construction.code,
                                                                CodeGenType::word);

                    generated_words.insert(
                        {
                            filtered_name,
                            {
                                .name = construction.name,
                                .locations = std::move(locations),
                                .constants = std::move(constants)
                            }
                        });
                }

                // JIT compile the script's top level function handler.
                auto script_name = filter_word_name(name);

                auto [ locations, constants ] = jit_compile(interpreter,
                                                            module,
                                                            context,
                                                            script_name,
                                                            code,
                                                            CodeGenType::script_body);

                // Finalize and optimize the module.
                auto ir_map = finalize_module(std::move(module), std::move(context));

                // Now we can extract and register all of the generated words.
                for (auto& [ word_name, generated_word ] : generated_words)
                {
                    auto handler = create_word_function(word_name,
                                                        std::move(ir_map[word_name]),
                                                        std::move(generated_word.locations),
                                                        std::move(generated_word.constants),
                                                        CodeGenType::word);

                    interpreter->replace_word(generated_word.name, handler);
                }

                TrackingMemoryManager::FunctionSizes.clear();

                // Return the script's top level function handler.
                return create_word_function(script_name,
                                            std::move(""),
                                            //std::move(""),
                                            std::move(locations),
                                            std::move(constants),
                                            CodeGenType::script_body);
            }


            std::string disassemble(uint64_t address, size_t size)
            {
                // Gether up the information needed for the disassembly engine to know how to work
                // for our runtime-target.

                // Start off by getting the target triple from the JIT engine.
                auto triple = jit->getTargetTriple();
                std::string triple_name = triple.str();

                // Make sure we can get the target information for the JIT engine.
                std::string error;

                auto target = llvm::TargetRegistry::lookupTarget(triple_name, error);
                if (!target)
                {
                    throw_error("Error finding target: " + error);
                }

                // Get the register information for this target.
                std::unique_ptr<const llvm::MCRegisterInfo>
                                                     reg_info(target->createMCRegInfo(triple_name));
                if (!reg_info)
                {
                    throw_error("Error creating MCRegisterInfo.");
                }

                // Get the target options for this target.
                llvm::MCTargetOptions options;
                std::unique_ptr<const llvm::MCAsmInfo> asm_info(target->createMCAsmInfo(*reg_info,
                                                                                        triple_name,
                                                                                        options));
                if (!asm_info)
                {
                    throw_error("Error creating MCAsmInfo for target" + triple_name + ".");
                }

                // Get the sub-target information for this target.
                std::unique_ptr<const llvm::MCSubtargetInfo> sti(
                                         target->createMCSubtargetInfo(triple_name, "generic", ""));
                if (!sti)
                {
                    throw_error("Error creating MCSubtargetInfo for target" + triple_name + ".");
                }

                // Create the MC context for this target.
                llvm::MCContext mc_context(triple, asm_info.get(), reg_info.get(), sti.get());
                std::unique_ptr<llvm::MCDisassembler> disassembler(
                                                    target->createMCDisassembler(*sti, mc_context));
                if (!disassembler)
                {
                    throw_error("Error creating MCDisassembler for target" + triple_name + ".");
                }

                // Get the instruction information for this target.
                std::unique_ptr<const llvm::MCInstrInfo> instr_info(target->createMCInstrInfo());

                // Create an instruction printer for this target.
                std::unique_ptr<llvm::MCInstPrinter> inst_printer(
                                                            target->createMCInstPrinter(triple,
                                                                                        0,
                                                                                        *asm_info,
                                                                                        *instr_info,
                                                                                        *reg_info));

                // Create a temporay non-owning array reference for the code.
                llvm::ArrayRef<uint8_t> code((uint8_t*)address, size);
                uint64_t index = address;

                // Create a buffer and a llvm string stream to hold the disassembly.
                std::string disassembly;
                llvm::raw_string_ostream stream(disassembly);

                // Go through the array of code and disassemble each instruction in it, advancing
                // the index by the size of the found instruction.
                while (index < address + size)
                {
                    // The next instruction and it's size.
                    llvm::MCInst inst;
                    uint64_t inst_size;

                    // Try to disassemble the next instruction.
                    if (disassembler->getInstruction(inst,
                                                     inst_size,
                                                     code.slice(index - address),
                                                     index,
                                                     llvm::nulls()))
                    {
                        // We found a valid instruction, so print it out.
                        stream << llvm::format("0x%016llx: ", index);
                        inst_printer->printInst(&inst, index, "", *sti, stream);
                        stream << "\n";

                        // Advance the index by the size of the instruction, with the size retrieved
                        // from the getInstruction call.
                        index += inst_size;
                    }
                    else
                    {
                        // If we couldn't disassemble the instruction, then just print out the
                        // address and that we couldn't disassemble it.  Then try to realign the
                        // index by one byte to try to find valid instructions again.
                        stream << llvm::format("0x%016llx:", index) << " <unknown>\n";
                        ++index;
                    }
                }

                // All done.
                return disassembly;
            }


            // Create the LLVM module, context, and builder for JITing code.
            std::tuple<std::unique_ptr<llvm::Module>,
                       std::unique_ptr<llvm::LLVMContext>>
                            create_jit_module_context(const std::string& name)
            {
                // Create the llvm module that will hold the JITed code.  Then register our helper
                // functions with it.
                auto context = std::make_unique<llvm::LLVMContext>();
                auto module = std::make_unique<llvm::Module>("sorth_module_" + name,
                                                             *context.get());
                register_jit_helpers(module, context);

                return { std::move(module), std::move(context) };
            }


            std::unordered_map<std::string,
                               std::string>
                finalize_module(std::unique_ptr<llvm::Module>&& module,
                                std::unique_ptr<llvm::LLVMContext>&& context)
            {
                // Capture the optimized IR for all the module's functions.
                std::unordered_map<std::string, std::string> ir_map;

                // Uncomment to print out the LLVM IR for the JITed function module but before being
                // optimized.
                // module->print(llvm::outs(), nullptr);

                // Make sure that the generated module is viable.
                std::string error_str;
                llvm::raw_string_ostream error_stream(error_str);

                if (llvm::verifyModule(*module, &error_stream))
                {
                    error_stream.flush();
                    throw_error("Module verification failed: " + error_str);
                }

                // Create the pass manager that will run the optimization passes on the module.
                llvm::PassBuilder pass_builder;
                llvm::LoopAnalysisManager loop_am;
                llvm::FunctionAnalysisManager function_am;
                llvm::CGSCCAnalysisManager cgsccam;
                llvm::ModuleAnalysisManager module_am;

                pass_builder.registerModuleAnalyses(module_am);
                pass_builder.registerCGSCCAnalyses(cgsccam);
                pass_builder.registerFunctionAnalyses(function_am);
                pass_builder.registerLoopAnalyses(loop_am);
                pass_builder.crossRegisterProxies(loop_am, function_am, cgsccam, module_am);

                llvm::ModulePassManager mpm =
                            pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

                // Now, run the optimization passes on the module.
                mpm.run(*module, module_am);

                // Uncomment to print out the LLVM IR for the JITed function module after being
                // optimized.
                // module->print(llvm::outs(), nullptr);

                // Capture Optimized IR for Each Function...
                for (auto &function : module->functions())
                {
                    if (!function.isDeclaration())
                    {
                        std::string func_ir;
                        llvm::raw_string_ostream rso(func_ir);
                        function.print(rso);
                        rso.flush();

                        ir_map[function.getName().str()] = func_ir;
                    }
                }

                // Commit our module to the JIT engine and let it get compiled.
                auto error = jit->addIRModule(llvm::orc::ThreadSafeModule(std::move(module),
                                                                          std::move(context)));

                if (error)
                {
                    std::string error_message;

                    llvm::raw_string_ostream stream(error_message);
                    llvm::logAllUnhandledErrors(std::move(error),
                                                stream,
                                                "Failed to jit module: ");
                    stream.flush();

                    throw_error(error_message);
                }

                return ir_map;
            }


            // Create a function handler for the JITed code.
            WordFunction create_word_function(const std::string& name,
                                              std::string&& function_ir,
                                              std::vector<Location>&& locations_,
                                              std::vector<Value>&& constants_,
                                              CodeGenType type)
            {
                auto locations = std::move(locations_);
                auto constants = std::move(constants_);

                // Get our generated function from the jit engine.
                auto symbol = jit->lookup(name);

                if (!symbol)
                {
                    throw_error("Failed to find JITed function symbol " + name + ".");
                }

                auto compiled_function = symbol->toPtr<void (*)(void*, // interpreter
                                                                const void*, // locations
                                                                const void*)>(); // constants


                // Create the function that will hold the JITed code and return it to the caller.
                auto handler_wrapper = [=, this](InterpreterPtr& interpreter) -> void
                    {
                        // The interpreter pointer is passed in as the first argument.  Convert it
                        // to a void pointer for the JITed code.  We do the same for the locations
                        // and constants vectors.
                        void* interpreter_ptr = static_cast<void*>(&interpreter);
                        const void* locations_ptr = static_cast<const void*>(&locations);
                        const void* constants_ptr = static_cast<const void*>(&constants);

                        // Clear any previous exception that may have occurred.
                        clear_last_exception();

                        // Mark our word's context for it's local variables.  But only if we're
                        // compiling a word, not a script.
                        if (type == CodeGenType::word)
                        {
                            interpreter->mark_context();
                        }

                        // Call the JITed function.
                        compiled_function(interpreter_ptr, locations_ptr, constants_ptr);

                        // Release the context, freeing up any local variables.
                        if (type == CodeGenType::word)
                        {
                            interpreter->release_context();
                        }

                        // If an exception occurred in the JITed code, re-throw it.
                        if (last_exception)
                        {
                            throw *last_exception;
                        }
                    };

                // Create a new handler object for our generated word.
                WordFunction handler = WordFunction::Handler(handler_wrapper);

                // Set the IR for the function.
                handler.set_ir(function_ir);

                // Set the generated disassembly for the function.  If we were able to properly get
                // it's address and size.
                auto [ address, size ] = TrackingMemoryManager::FunctionSizes[name];

                if ((address) && (size > 0))
                {
                    // We have a proper address and size, so disassemble the function.
                    handler.set_asm_code(disassemble(address, size));
                }

                // Return the new function handler.
                return handler;
            }


            // JIT compile the given byte-code block into a native function handler.
            std::tuple<std::vector<Location>,
                       std::vector<Value>> jit_compile(InterpreterPtr& interpreter,
                                                       std::unique_ptr<llvm::Module>& module,
                                                       std::unique_ptr<llvm::LLVMContext>& context,
                                                       const std::string& name,
                                                       const ByteCode& code,
                                                       CodeGenType type)
            {
                // Create the instruction builder for the JITed code.
                llvm::IRBuilder<> builder(*context.get());

                // Gather up our basic types, and then create the function type for the JITed code.
                auto void_type = llvm::Type::getVoidTy(*context.get());
                auto ptr_type = llvm::PointerType::getUnqual(void_type);

                auto function_type = llvm::FunctionType::get(void_type,
                                                             { ptr_type, ptr_type, ptr_type },
                                                             false);

                // Now, create the function that will hold the JITed code.
                auto function = llvm::Function::Create(function_type,
                                                       llvm::Function::ExternalLinkage,
                                                       name,
                                                       module.get());

                // Keep track of source locations found in the byte-code so that we can later keep
                // track of where exceptions occurred.
                std::vector<Location> locations;

                // Keep track of constant values that are to complex to be inlined in the generated
                // code.
                std::vector<Value> constants;

                // Generate the body of the JITed function.
                generate_function_body(interpreter,
                                       module,
                                       builder,
                                       function,
                                       context,
                                       name,
                                       locations,
                                       constants,
                                       code);

                return { std::move(locations), std::move(constants) };
            }


            // Generate the body of the JITed function.  We do this by walking the byte-code block
            // and generating the appropriate llvm instructions for each operation.
            void generate_function_body(InterpreterPtr& interpreter,
                                        std::unique_ptr<llvm::Module>& module,
                                        llvm::IRBuilder<>& builder,
                                        llvm::Function* function,
                                        std::unique_ptr<llvm::LLVMContext>& context,
                                        const std::string& name,
                                        std::vector<Location>& locations,
                                        std::vector<Value>& constants,
                                        const ByteCode& code)
            {
                // Gather some types we'll need.
                auto bool_type = llvm::Type::getInt1Ty(*context.get());
                auto int64_type = llvm::Type::getInt64Ty(*context.get());
                auto double_type = llvm::Type::getDoubleTy(*context.get());
                auto char_type = llvm::Type::getInt1Ty(*context.get());


                // Keep track of the basic blocks we create for the jump targets.
                auto blocks = std::unordered_map<size_t, llvm::BasicBlock*>();
                auto auto_jump_blocks = std::unordered_map<size_t, std::pair<llvm::BasicBlock*,
                                                                             llvm::BasicBlock*>>();

                // Create the entry block of the function.
                auto entry_block = llvm::BasicBlock::Create(*context, "entry_block", function);
                builder.SetInsertPoint(entry_block);

                // Get a reference to the interpreter pointer argument.
                auto interpreter_ptr = function->getArg(0);
                auto location_arr_ptr = function->getArg(1);
                auto constant_arr_ptr = function->getArg(2);

                // Keep track of any loop and catch block markers.
                std::vector<std::pair<size_t, size_t>> loop_markers;
                std::vector<size_t> catch_markers;

                // Keep track of any jump targets that are the target of a catch block.
                std::set<size_t> catch_target_markers;

                // Take a first pass through the code to create the blocks that we'll need.
                auto block_index = 1;

                for (size_t i = 0; i < code.size(); ++i)
                {
                    if (   (code[i].id == Instruction::Id::execute)
                        || (code[i].id == Instruction::Id::jump_target)
                        || (code[i].id == Instruction::Id::def_variable)
                        || (code[i].id == Instruction::Id::def_constant)
                        || (code[i].id == Instruction::Id::read_variable)
                        || (code[i].id == Instruction::Id::write_variable)
                        || (code[i].id == Instruction::Id::jump_loop_start)
                        || (code[i].id == Instruction::Id::jump_loop_exit))
                    {
                        // These instructions are the ones that can start a new block.  In the case
                        // of a jump target it is an explicit start of a block.  The jump
                        // instruction is also the implicit end of a block.  For  the other
                        // instructions it is an implicit start of a block.  Usually for exception
                        // checking these instructions can call a function that can potentially
                        // raise an exception.  So we create a new block for each of these
                        // instructions.
                        std::stringstream stream;

                        stream << "block_" << block_index;
                        ++block_index;

                        blocks[i] = llvm::BasicBlock::Create(*context, stream.str(), function);
                    }
                    else if (   (code[i].id == Instruction::Id::jump_if_zero)
                             || (code[i].id == Instruction::Id::jump_if_not_zero))
                    {
                        // These instructions have two jumps.  One jump for the pop error check and
                        // one for the actual jump.  So we create two blocks for each of these jump
                        // instructions.
                        std::stringstream stream;

                        stream << "block_" << block_index;
                        ++block_index;

                        auto block_a = llvm::BasicBlock::Create(*context, stream.str(), function);

                        std::stringstream stream_b;

                        stream_b << "block_" << block_index;
                        ++block_index;

                        auto block_b = llvm::BasicBlock::Create(*context, stream_b.str(), function);

                        auto_jump_blocks[i] = { block_a, block_b };
                    }
                }

                // // Create the end block of the function.
                auto exit_block = llvm::BasicBlock::Create(*context, "exit_block", function);

                // On second pass generate the actual code.
                for (size_t i = 0; i < code.size(); ++i)
                {
                    // Check to see if the current instruction has a location associated with it.
                    if (code[i].location)
                    {
                        // Save the location in our locations vector.
                        auto& location = code[i].location.value();
                        auto index = locations.size();

                        locations.push_back(location);

                        // Generate the code to update the location in the interpreter.
                        auto index_const = llvm::ConstantInt::get(int64_type, index);
                        builder.CreateCall(handle_set_location_fn,
                                           { interpreter_ptr, location_arr_ptr, index_const});
                    }

                    switch (code[i].id)
                    {
                        case Instruction::Id::def_variable:
                            {
                                // Get the variable name and create a string constant in the module
                                // for it.
                                auto name = code[i].value.as_string(interpreter);
                                auto name_ptr = define_string_constant(name,
                                                                       builder,
                                                                       module,
                                                                       context);

                                // Call the handle_define_variable function to define the variable
                                // in the interpreter.
                                auto result = builder.CreateCall(handle_define_variable_fn,
                                                                 { interpreter_ptr, name_ptr });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, blocks[i]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::def_constant:
                            {
                                // Get the constant name and create a string constant in the module
                                // for it.
                                auto name = code[i].value.as_string(interpreter);
                                auto name_ptr = define_string_constant(name,
                                                                       builder,
                                                                       module,
                                                                       context);

                                // Call the handle_define_constant function to define the constant
                                // in the interpreter.
                                auto result = builder.CreateCall(handle_define_constant_fn,
                                                                 { interpreter_ptr, name_ptr });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, blocks[i]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::read_variable:
                            {
                                // Call the handle_read_variable function to read the variable from
                                // the interpreter.
                                auto result = builder.CreateCall(handle_read_variable_fn,
                                                                 { interpreter_ptr });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, blocks[i]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::write_variable:
                            {
                                // Call the handle_write_variable function to write the variable to
                                // the interpreter.
                                auto result = builder.CreateCall(handle_write_variable_fn,
                                                                 { interpreter_ptr });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, blocks[i]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::execute:
                            {
                                // Get the name or index of the word to execute.
                                auto& value = code[i].value;

                                // Capture the result of teh execute call instruction.
                                llvm::CallInst* result = nullptr;

                                if (value.is_string())
                                {
                                    // The value is a string, so execute the word based on it's
                                    // name.
                                    auto name = value.as_string(interpreter);
                                    auto name_ptr = define_string_constant(name,
                                                                           builder,
                                                                           module,
                                                                           context);

                                    result = builder.CreateCall(handle_word_execute_name_fn,
                                                                { interpreter_ptr, name_ptr });
                                }
                                else if (value.is_numeric())
                                {
                                    // The value is a number, so execute the word based on it's
                                    // handler index.
                                    auto index = value.as_integer(interpreter);
                                    auto int_const = llvm::ConstantInt::get(int64_type, index);
                                    result = builder.CreateCall(handle_word_execute_index_fn,
                                                                { interpreter_ptr, int_const });
                                }
                                else
                                {
                                    throw_error("Can not execute unexpected value type.");
                                }

                                // Check the result of the call instruction and branch to the next
                                // if no errors were raised, otherwise branch to the either the
                                // exit block or the exception handler block.
                                auto next_block = blocks[i];

                                // Jump to the catch block if there is one, or the exit block if
                                // not.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto cmp = builder.CreateICmpNE(result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, next_block);

                                // We're done with the current block, so move on to the next one.
                                builder.SetInsertPoint(next_block);
                            }
                            break;

                        case Instruction::Id::word_index:
                            {
                                // Make sure we have a name for lookup.
                                if (!code[i].value.is_string())
                                {
                                    throw_error("Expected a numeric value for word_index.");
                                }

                                // See if we can find the word in the interpreter at compile time.
                                // If we can, we'll generate the code to push it's index on the
                                // stack, if we can't we'll generate code to try again at runtime.
                                auto name = code[i].value.as_string(interpreter);
                                auto [ found, word_info ] = interpreter->find_word(name);

                                if (found)
                                {
                                    auto int_const = llvm::ConstantInt::get(int64_type,
                                                                           word_info.handler_index);
                                    builder.CreateCall(handle_push_int_fn,
                                                       { interpreter_ptr, int_const });
                                }
                                else
                                {
                                    auto name_constant = define_string_constant(name,
                                                                                builder,
                                                                                module,
                                                                                context);
                                    builder.CreateCall(handle_word_index_name_fn,
                                                       { interpreter_ptr, name_constant });
                                }
                            }
                            break;

                        case Instruction::Id::word_exists:
                            {
                                // Make sure we have a name for lookup.
                                if (!code[i].value.is_string())
                                {
                                    throw_error("Expected a numeric value for word_index.");
                                }

                                // See if we can find the word in the interpreter at compile time.
                                // If we can, we'll generate the code to push a true value onto the
                                // stack, if we can't we'll generate code to try again at runtime.
                                auto name = code[i].value.as_string(interpreter);
                                auto [ found, _ ] = interpreter->find_word(name);

                                if (found)
                                {
                                    auto true_const = llvm::ConstantInt::get(bool_type, true);
                                    builder.CreateCall(handle_push_bool_fn,
                                                       { interpreter_ptr, true_const });
                                }
                                else
                                {
                                    auto name_ptr = define_string_constant(name,
                                                                           builder,
                                                                           module,
                                                                           context);

                                    builder.CreateCall(handle_word_exists_name_fn,
                                                       { interpreter_ptr, name_ptr });
                                }
                            }
                            break;

                        case Instruction::Id::push_constant_value:
                            {
                                // Get the value to push from the instruction.
                                auto& value = code[i].value;

                                // Check the type of the value.  If it's one of the simple types
                                // directly generate the code to push it's value onto the stack.
                                if (value.is_bool())
                                {
                                    auto bool_value = value.as_bool();
                                    auto bool_const = llvm::ConstantInt::get(bool_type, bool_value);
                                    builder.CreateCall(handle_push_bool_fn,
                                                       { interpreter_ptr, bool_const });
                                }
                                else if (value.is_integer())
                                {
                                    auto int_value = value.as_integer(interpreter);
                                    auto int_const = llvm::ConstantInt::get(int64_type, int_value);
                                    builder.CreateCall(handle_push_int_fn,
                                                       { interpreter_ptr, int_const });
                                }
                                else if (value.is_float())
                                {
                                    auto double_value = value.as_float(interpreter);
                                    auto double_const = llvm::ConstantFP::get(double_type,
                                                                              double_value);
                                    builder.CreateCall(handle_push_double_fn,
                                                       { interpreter_ptr, double_const });
                                }
                                else if (value.is_string())
                                {
                                    auto string_value = value.as_string(interpreter);
                                    auto string_ptr = define_string_constant(string_value,
                                                                             builder,
                                                                             module,
                                                                             context);
                                    builder.CreateCall(handle_push_string_fn,
                                                       { interpreter_ptr, string_ptr });
                                }
                                else
                                {
                                    // The value is a complex type that we can't inline in the JITed
                                    // code directly.  So we'll add it to the constants array and
                                    // generate code to push the value from the array onto the
                                    // stack.
                                    auto index = constants.size();
                                    constants.push_back(value);

                                    auto index_const = builder.getInt64(index);
                                    builder.CreateCall(handle_push_value_fn,
                                                       { interpreter_ptr,
                                                         constant_arr_ptr,
                                                         index_const });
                                }
                            }
                            break;

                        case Instruction::Id::mark_loop_exit:
                            {
                                // Capture the start and end indexes of the loop for later use.
                                auto start_index = i + 1;
                                auto end_index = i + code[i].value.as_integer(interpreter);

                                loop_markers.push_back({ start_index, end_index });
                            }
                            break;

                        case Instruction::Id::unmark_loop_exit:
                            {
                                // Clear the current loop markers.
                                loop_markers.pop_back();
                            }
                            break;

                        case Instruction::Id::mark_catch:
                            {
                                // Capture the index of the catch block for later use.
                                auto target_index = i + code[i].value.as_integer(interpreter);
                                catch_markers.push_back(target_index);
                                catch_target_markers.insert(target_index);
                            }
                            break;

                        case Instruction::Id::unmark_catch:
                            {
                                // Clear the current catch markers so that we don't jump to then.
                                // Note that we leave the catch_target_markers set alone so that we
                                // can generate the code to push the exception onto the stack when
                                // we reach the catch target instruction.
                                catch_markers.pop_back();
                            }
                            break;

                        case Instruction::Id::mark_context:
                            {
                                // Call the handle_manage_context function to mark the context in
                                // the interpreter.
                                auto bool_const = llvm::ConstantInt::get(bool_type, true);
                                builder.CreateCall(handle_manage_context_fn,
                                                   { interpreter_ptr, bool_const });
                            }
                            break;

                        case Instruction::Id::release_context:
                            {
                                // Call the handle_manage_context function to release the context in
                                // the interpreter.
                                auto bool_const = llvm::ConstantInt::get(bool_type, false);
                                builder.CreateCall(handle_manage_context_fn,
                                                   { interpreter_ptr, bool_const });
                            }
                            break;

                        case Instruction::Id::jump:
                            {
                                // Jump to the target block.
                                auto index = i + code[i].value.as_integer(interpreter);
                                builder.CreateBr(blocks[index]);
                            }
                            break;

                        case Instruction::Id::jump_if_zero:
                            {
                                // Convert the relative index to an absolute index.
                                auto index = i + code[i].value.as_integer(interpreter);

                                // Allocate a bool to hold the test value.  Then call the
                                // handle_pop_bool function to get the value from the stack.
                                auto test_value = builder.CreateAlloca(bool_type);
                                auto pop_result = builder.CreateCall(handle_pop_bool_fn,
                                                                   { interpreter_ptr, test_value });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                // Get the two blocks that we'll jump to based on the test value.
                                auto [ a, b ] = auto_jump_blocks[i];

                                auto cmp = builder.CreateICmpNE(pop_result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, a);

                                // The pop was successful, so switch to the next block and generate
                                // the code to perform the jump based on the test value.
                                builder.SetInsertPoint(a);

                                // Jump to the 'success' block if the test value is false, otherwise
                                // jump to the 'fail' block.
                                auto read_value = builder.CreateLoad(bool_type, test_value);
                                builder.CreateCondBr(read_value, b, blocks[index]);
                                builder.SetInsertPoint(b);
                            }
                            break;

                        case Instruction::Id::jump_if_not_zero:
                            {
                                // Convert the relative index to an absolute index.
                                auto index = i + code[i].value.as_integer(interpreter);

                                // Allocate a bool to hold the test value.  Then call the
                                // handle_pop_bool function to get the value from the stack.
                                auto test_value = builder.CreateAlloca(bool_type);
                                auto pop_result = builder.CreateCall(handle_pop_bool_fn,
                                                                   { interpreter_ptr, test_value });

                                // Check the result of the call instruction and branch to the next
                                // block if no errors were raised, otherwise branch to the either
                                // the exit block or the exception handler block.
                                auto error_block = catch_markers.empty()
                                                   ? exit_block
                                                   : blocks[catch_markers.back()];

                                auto [ a, b ] = auto_jump_blocks[i];

                                auto cmp = builder.CreateICmpNE(pop_result, builder.getInt64(0));
                                builder.CreateCondBr(cmp, error_block, a);

                                // The pop was successful, so switch to the next block and generate
                                // the code to perform the jump based on the test value.
                                builder.SetInsertPoint(a);

                                // Jump to the 'success' block if the test value is true, otherwise
                                // jump to the 'fail' block.
                                auto read_value = builder.CreateLoad(bool_type, test_value);
                                builder.CreateCondBr(read_value, blocks[index], b);
                                builder.SetInsertPoint(b);
                            }
                            break;

                        case Instruction::Id::jump_loop_start:
                            {
                                // Jump to the start block of the loop.
                                auto start_index = loop_markers.back().first;

                                builder.CreateBr(blocks[start_index]);
                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::jump_loop_exit:
                            {
                                // Jump to the end block of the loop.
                                auto end_index = loop_markers.back().second;
                                builder.CreateBr(blocks[end_index]);

                                builder.SetInsertPoint(blocks[i]);
                            }
                            break;

                        case Instruction::Id::jump_target:
                            {
                                // Make sure that the current block has a terminator instruction, if
                                // not, add one to jump to the next block.  This is because this
                                // would be a natural follow through in the original byte-code.
                                if (builder.GetInsertBlock()->getTerminator() == nullptr)
                                {
                                    builder.CreateBr(blocks[i]);
                                }

                                // We're done with the current block, switch over to the next.
                                builder.SetInsertPoint(blocks[i]);


                                // If this is a catch target, we need to generate the code to push
                                // the exception onto the stack and clear the last exception.
                                if (   (!catch_target_markers.empty())
                                    && (catch_target_markers.find(i) != catch_target_markers.end()))
                                {
                                    builder.CreateCall(handle_push_last_exception_fn,
                                                       { interpreter_ptr });
                                }
                            }
                            break;
                    }
                }

                // Make sure that the last block has a terminator instruction, if not, add one to
                // jump to the exit block.
                if (builder.GetInsertBlock()->getTerminator() == nullptr)
                {
                    builder.CreateBr(exit_block);
                }

                // We're done with the last user code block, so switch over to the exit block and
                // generate the code to return from the function.
                builder.SetInsertPoint(exit_block);
                builder.CreateRetVoid();
            }


            // Set the last exception that occurred.
            static void set_last_exception(const std::runtime_error& error)
            {
                last_exception = error;
            }


            // Clear the last exception that occurred.
            static void clear_last_exception()
            {
                last_exception.reset();
            }


            // Handle popping a boolean value from the interpreter stack for the JITed code.
            static int64_t handle_pop_bool(void* interpreter_ptr, bool* value)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                    *value = interpreter->pop_as_bool();
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Filter word names to be acceptable for use as llvm function names.
            std::string filter_word_name(const std::string& name)
            {
                static std::atomic<int64_t> index = 0;

                std::stringstream stream;
                auto current = index.fetch_add(1, std::memory_order_relaxed);

                stream << "__fn_";

                for (auto c : name)
                {
                    switch (c)
                    {
                        case '@':   stream << "_at_";       break;
                        case '\'':  stream << "_prime_";    break;
                        case '"':   stream << "_quote_";    break;
                        case '%':   stream << "_percent_";  break;
                        case '!':   stream << "_bang_";     break;
                        case '?':   stream << "_question_"; break;

                        default:    stream << c;            break;
                    }
                }

                stream << "_index_" << std::setw(6) << std::setfill('0') << current
                       << "_";

                return stream.str();
            }


            // Define a string constant in the llvm module for accessing from the JITed code.
            llvm::Value* define_string_constant(const std::string& text,
                                                llvm::IRBuilder<>& builder,
                                                std::unique_ptr<llvm::Module>& module,
                                                std::unique_ptr<llvm::LLVMContext>& context)
            {
                // Get our types.
                auto char_type = llvm::Type::getInt8Ty(*context.get());
                auto char_ptr_type = llvm::PointerType::getUnqual(char_type);

                // Create the constant data array for the string then create a global variable to
                // hold it.
                auto string_constant = llvm::ConstantDataArray::getString(*context.get(),
                                                                          text,
                                                                          true);

                auto global = new llvm::GlobalVariable(*module,
                                                       string_constant->getType(),
                                                       true,
                                                       llvm::GlobalValue::PrivateLinkage,
                                                       string_constant);

                // Create a pointer to the global variable for the string constant.
                llvm::Value* string_ptr = builder.CreatePointerCast(global, char_ptr_type);

                return string_ptr;
            }


            // Handle setting the current source location in the interpreter for the JITed code.
            static void handle_set_location(void* interpreter_ptr,
                                            void* location_array_ptr,
                                            size_t index)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                auto& location_array = *static_cast<std::vector<Location>*>(location_array_ptr);
                auto& location = location_array[index];

                interpreter->set_location(location);
            }


            // Handle the context management for the JITed code.  If passed a true value, mark the
            // context, otherwise release it.
            static void handle_manage_context(void* interpreter_ptr, bool is_marking)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                // TODO: Add error checking to make sure that these calls are balanced.

                if (is_marking)
                {
                    interpreter->mark_context();
                }
                else
                {
                    interpreter->release_context();
                }
            }


            // Handle defining a new variable in the interpreter for the JITed code.
            static int64_t handle_define_variable(void* interpreter_ptr, const char* name)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                    interpreter->define_variable(name);
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle defining a new constant in the interpreter for the JITed code.
            static int64_t handle_define_constant(void* interpreter_ptr, const char* name)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                    auto value = interpreter->pop();

                    interpreter->define_constant(name, value);
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle a variable read operation for the JITed code.
            static int64_t handle_read_variable(void* interpreter_ptr)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                    auto index = interpreter->pop_as_integer();

                    interpreter->push(interpreter->read_variable(index));
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle a variable write operation for the JITed code.
            static int64_t handle_write_variable(void* interpreter_ptr)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                    auto index = interpreter->pop_as_integer();
                    auto value = interpreter->pop();

                    interpreter->write_variable(index, value);
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            static void handle_push_last_exception(void* interpreter_ptr)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                if (last_exception)
                {
                    // Push the exception message onto the interpreter stack, and clear the last
                    // exception because user code has handled it.
                    interpreter->push(last_exception->what());
                    clear_last_exception();
                }
                else
                {
                    interpreter->push("");
                }
            }


            // Handle pushing a boolean value onto the interpreter stack for the JITed code.
            static void handle_push_bool(void* interpreter_ptr, bool value)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                interpreter->push(value);
            }


            // Handle pushing an integer value onto the interpreter stack for the JITed code.
            static void handle_push_int(void* interpreter_ptr, int64_t value)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                interpreter->push(value);
            }


            // Handle pushing a double value onto the interpreter stack for the JITed code.
            static void handle_push_double(void* interpreter_ptr, double value)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                interpreter->push(value);
            }


            // Handle pushing a string value onto the interpreter stack for the JITed code.
            static void handle_push_string(void* interpreter_ptr, const char* value)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                interpreter->push(std::string(value));
            }


            static void handle_push_value(void* interpreter_ptr, void* array_ptr, int64_t index)
            {
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                auto& array = *static_cast<std::vector<Value>*>(array_ptr);
                auto& value = array[index];

                interpreter->push(value.deep_copy());
            }


            // Handle executing a word by name for the JITed code.
            static int64_t handle_word_execute_name(void* interpreter_ptr, const char* name)
            {
                int64_t result = 0;
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                try
                {
                    auto location = interpreter->get_current_location();

                    interpreter->call_stack_push(name, location);
                    interpreter->execute_word(name);
                    interpreter->call_stack_pop();
                }
                catch (std::runtime_error& error)
                {
                    interpreter->call_stack_pop();

                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle executing a word by index for the JITed code.
            static int64_t handle_word_execute_index(void* interpreter_ptr, int64_t index)
            {
                int64_t result = 0;
                auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);

                try
                {
                    auto& info = interpreter->get_handler_info(index);

                    interpreter->call_stack_push(info);
                    info.function(interpreter);
                    interpreter->call_stack_pop();
                }
                catch (std::runtime_error& error)
                {
                    interpreter->call_stack_pop();

                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle looking up a word index by name for the JITed code.
            static int64_t handle_word_index_name(void* interpreter_ptr, const char* name)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                    auto [ found, word ] = interpreter->find_word(name);

                    if (!found)
                    {
                        throw_error(interpreter, "Word " + std::string(name) + " not found.");
                    }

                    interpreter->push((int64_t)word.handler_index);
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Handle looking up a word index by name for the JITed code.
            static int64_t handle_word_exists_name(void* interpreter_ptr, const char* name)
            {
                int64_t result = 0;

                try
                {
                    auto& interpreter = *static_cast<InterpreterPtr*>(interpreter_ptr);
                    auto [ found, _ ] = interpreter->find_word(name);

                    interpreter->push(found);
                }
                catch (std::runtime_error& error)
                {
                    set_last_exception(error);
                    result = -1;
                }

                return result;
            }


            // Simple handler that does nothing, used in the case we're given an empty byte-code
            // block.
            static void null_handler(InterpreterPtr&)
            {
                // Nothing to do.
            }
        };


        // The storage for the last exception that occurred in the JITed code.
        thread_local std::optional<std::runtime_error> JitEngine::last_exception;


        // Only one thread can be JIT compiling at a time.
        std::mutex jit_lock;

        // The one instance of the jit engine.
        JitEngine jit_engine;


    }


    // Lock the JIT engine and JIT compile the given immediate word.
    WordFunction jit_immediate_word(InterpreterPtr& Interpreter,
                                    const Construction& construction)
    {
        std::lock_guard<std::mutex> lock(jit_lock);

        return jit_engine.jit_bytecode(Interpreter, construction);

    }


    // JIt compile the given byte-code block into the script's top level function handler.  While
    // doing that we'll also JIT compile all of the script's non-immediate words that have been
    // cached during the byte-code compilation phase.
    //
    // We return the handler for the top level script function, all words are registered in the
    // interpreter's dictionary automaticaly.
    WordFunction jit_module(InterpreterPtr& Interpreter,
                            const std::string& name,
                            const ByteCode& code,
                            const std::map<std::string, Construction>& word_jit_cache)
    {
        std::lock_guard<std::mutex> lock(jit_lock);

        return jit_engine.jit_bytecode(Interpreter, name, code, word_jit_cache);
    }

}


#endif

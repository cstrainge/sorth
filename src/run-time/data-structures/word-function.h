
#pragma once


namespace sorth::internal
{


    class SORTH_API WordFunction
    {
        public:
            using Handler = std::function<void(InterpreterPtr&)>;

        private:
            Handler function;

            std::optional<ByteCode> byte_code;
            std::optional<std::string> ir;
            std::optional<std::string> asm_code;

        public:
            WordFunction();
            WordFunction(const Handler& function);
            WordFunction(const WordFunction& word_function);
            WordFunction(WordFunction&& word_function);
            ~WordFunction();

        public:
            WordFunction& operator =(const Handler& raw_function);
            WordFunction& operator =(const WordFunction& word_function);
            WordFunction& operator =(WordFunction&& word_function);

            void operator ()(InterpreterPtr& interpreter);

        public:
            Handler get_function() const;

            void set_byte_code(const ByteCode& code);
            void set_byte_code(const ByteCode&& code);
            const std::optional<ByteCode>& get_byte_code() const;

            void set_ir(const std::string& code);
            const std::optional<std::string>& get_ir() const;

            void set_asm_code(const std::string& code);
            const std::optional<std::string>& get_asm_code() const;
    };


    void pretty_print_bytecode(InterpreterPtr& interpreter,
                               const ByteCode& code,
                               std::ostream& stream);


}

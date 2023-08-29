
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        char process_escape_literal(SourceBuffer& source_code)
        {
            char next = source_code.next();

            switch (next)
            {
                case 'n':
                    next = '\n';
                    break;

                case 'r':
                    next = '\r';
                    break;

                case 't':
                    next = '\t';
                    break;

                case '0':
                    {
                        Location start = source_code.current_location();
                        std::string number_string;

                        while (source_code)
                        {
                            next = source_code.peek_next();

                            if ((next >= '0') && (next <= '9'))
                            {
                                number_string += next;
                                source_code.next();
                            }
                            else
                            {
                                break;
                            }
                        }

                        auto numeric = std::stoi(number_string);

                        throw_error_if(numeric >= 256,
                                       start,
                                       "Numeric character literal out of range.");

                        next = (char)numeric;
                    }
                    break;
            }

            return next;
        }


        std::string process_string(SourceBuffer& source_code)
        {
            Location start = source_code.current_location();

            std::string new_string;
            char next = 0;

            source_code.next();

            while (source_code)
            {
                next = source_code.next();

                if (next == '"')
                {
                    break;
                }

                if (next == '\\')
                {
                    next = process_escape_literal(source_code);

                }
                new_string += next;
            }

            throw_error_if(next != '"', start, "Missing end of string literal.");

            return new_string;
        }


    }


    std::ostream& operator <<(std::ostream& stream, Token token)
    {
        stream << token.location << ": " << token.text;

        return stream;
    }


     TokenList tokenize(SourceBuffer& source_code)
    {
        TokenList tokens;

        auto get_while = [&](std::function<bool(char)> test) -> std::string
            {
                std::string new_string;
                char next = source_code.peek_next();

                while (    (source_code)
                        && (test(next)))
                {
                    new_string += source_code.next();
                    next = source_code.peek_next();
                }

                return new_string;
            };

        auto is_whitespace = [](char next) -> bool
            {
                return    (next == ' ')
                       || (next == '\t')
                       || (next == '\n');
            };

        auto is_numeric = [](const std::string& text) -> bool
            {
                if ((text[0] >= '0') && (text[0] <= '9'))
                {
                    return true;
                }

                if (   (text[0] == '-')
                    || (text[0] == '+'))
                {
                    return (text[1] >= '0') && (text[1] <= '9');
                }

                return false;
            };

        auto skip_whitespace = [&]()
            {
                char next = source_code.peek_next();

                while (source_code && (is_whitespace(next)))
                {
                    source_code.next();
                    next = source_code.peek_next();
                }
            };

        while (source_code)
        {
            skip_whitespace();

            if (!source_code)
            {
                break;
            }

            std::string text;
            Token::Type type = Token::Type::word;
            Location location = source_code.current_location();

            if (source_code.peek_next() == '"')
            {
                type = Token::Type::string;
                text = process_string(source_code);
            }
            else
            {
                text = get_while([&](char next) -> bool { return is_whitespace(next) == false; });
            }

            if (   (type != Token::Type::string)
                && (is_numeric(text)))
            {
                type = Token::Type::number;
            }

            tokens.push_back({.type = type, .location = location, .text = text });
        }

        return tokens;
    }


}

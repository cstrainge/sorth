
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        bool is_whitespace(char next)
        {
            return    (next == ' ')
                    || (next == '\t')
                    || (next == '\n');
        }

        void skip_whitespace(SourceBuffer& source_code)
        {
            char next = source_code.peek_next();

            while (source_code && is_whitespace(next))
            {
                source_code.next();
                next = source_code.peek_next();
            }
        }


        void skip_whitespace_until_column(SourceBuffer& source_code, size_t target_column)
        {
            char next = source_code.peek_next();

            while (   source_code
                   && is_whitespace(next)
                   && (source_code.current_location().get_column() < target_column))
            {
                source_code.next();
                next = source_code.peek_next();
            }
        }


        void append_newlines(std::string& text, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                text += '\n';
            }
        }


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


        std::string process_multi_line_string(const Location& start, SourceBuffer& source_code)
        {
            // Extract the *.
            source_code.next();

            // Extract first whitespace until we get to first real text, and record the colum we are
            // at now.
            skip_whitespace(source_code);

            auto target_column = source_code.current_location().get_column();
            char next = 0;

            std::string new_string;

            while (source_code)
            {
                next = source_code.next();

                // Found an asterisk, check to see if the next char is a quote.  If it is, we're
                // done with this string.
                if (next == '*')
                {
                    if (source_code.peek_next() == '"')
                    {
                        next = source_code.next();
                        break;
                    }
                    else
                    {
                        new_string += next;
                    }
                }
                else if (next == '\\')
                {
                    // Process the escaped character.
                    new_string += process_escape_literal(source_code);
                }
                else if (next == '\n')
                {
                    new_string += next;

                    // We're on a new line, so get rid of the whitespace until we either find text
                    // or reach the column we're looking for.  Any whitespace after that column will
                    // be included in the string.  Any skiped new lines should be added to the
                    // string.
                    auto start_line = source_code.current_location().get_line();

                    skip_whitespace_until_column(source_code, target_column);

                    auto current_line = source_code.current_location().get_line();

                    if (current_line > (start_line))
                    {
                        append_newlines(new_string, current_line - (start_line));
                    }
                }
                else
                {
                    // No special processing needed for this character.
                    new_string += next;
                }
            }

            return new_string;
        }


        std::string process_string(SourceBuffer& source_code)
        {
            Location start = source_code.current_location();

            std::string new_string;

            source_code.next();

            if (source_code.peek_next() == '*')
            {
                new_string = process_multi_line_string(start, source_code);
            }
            else
            {
                char next = 0;

                while (source_code)
                {
                    next = source_code.next();

                    if (next == '"')
                    {
                        break;
                    }

                    throw_error_if(next == '\n', start, "Unexpected new line in string literal.");

                    if (next == '\\')
                    {
                        next = process_escape_literal(source_code);

                    }
                    new_string += next;
                }

                throw_error_if(next != '"', start, "Missing end of string literal.");
            }

            return new_string;
        }


    }


    std::ostream& operator <<(std::ostream& stream, Token token)
    {
        stream << token.location << ": " << token.text;

        return stream;
    }


    bool operator ==(const Token& lhs, const Token& rhs)
    {
        if (lhs.type != rhs.type)
        {
            return false;
        }

        if (lhs.text != rhs.text)
        {
            return false;
        }

        // We ignore location for this comparison.

        return true;
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

        while (source_code)
        {
            skip_whitespace(source_code);

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

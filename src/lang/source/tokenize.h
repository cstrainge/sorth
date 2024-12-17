
#pragma once


namespace sorth::internal
{


    // Take the source in a source buffer and convert it into a list of tokens, or words really.
    // This is used by the native compiler and also the interpreted code to manipulate the language
    // itself.  One of the fun things about Forth is that it can help define itself.

    struct Token
    {
        // The tokenizer's best guess of the meaning of the given token.  This is really optional as
        // it may be reinterpreted later.
        enum class Type
        {
            number,
            string,
            word
        };

        Type type;          // What kind of token do we think this is?
        Location location;  // Where in the source was this token found?
        std::string text;   // The actual token itself.
    };


    using TokenList = std::vector<Token>;


    std::ostream& operator <<(std::ostream& stream, Token token);

    bool operator ==(const Token& lhs, const Token& rhs);


    TokenList tokenize(SourceBuffer& source_code);


}

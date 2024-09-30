
#pragma once


namespace sorth::internal
{


    using WordFunction = std::function<void(InterpreterPtr&)>;

    struct WordHandlerInfo
    {
        std::string name;
        WordFunction function;
        Location definition_location;
    };

    using WordList = ContextualList<WordHandlerInfo>;


    using StringPtr = std::shared_ptr<std::string>;


    // The Forth dictionary.  Handlers for Forth words are not stored directly in the dictionary.
    // Instead they are stored in their own list and the index and any important flags are what is
    // stored in the dictionary directly.
    //
    // Also note that the dictionary is implemented as a stack of dictionaries.  This allows the
    // Forth to contain scopes.  If a word is redefined in a higher scope, it effectively replaces
    // that word until that scope is released.
    struct Word
    {
        bool is_immediate;        // Should this word be executed at compile or at run time?  True
                                  // indicates that the word is executed at compile time.
        bool is_scripted;         // Was this word defined in Forth?
        bool is_hidden;           // Is this word hidden from the word dictionary?
        StringPtr description;    // Quick one line description of this word.
        StringPtr signature;      // How does this word interact with the stack?

        Location location;        // Location this word was defined.

        size_t handler_index;     // Index of the handler to execute.
    };


    class Dictionary
    {
        private:
            using SubDictionary = std::unordered_map<std::string, Word>;
            using DictionaryStack = std::list<SubDictionary>;

            DictionaryStack stack;

        public:
            Dictionary();

        public:
            void insert(const std::string& text, const Word& value);
            std::tuple<bool, Word> find(const std::string& word) const;

        public:
            void mark_context();
            void release_context();

        public:
            std::map<std::string, Word> get_merged_dictionary() const;

        protected:
            friend std::vector<std::string> inverse_lookup_list(const Dictionary& dictionary,
                                                                const WordList& word_handlers);
            friend std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);
    };


    std::vector<std::string> inverse_lookup_list(const Dictionary& dictionary,
                                                 const WordList& word_handlers);


    std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);


}

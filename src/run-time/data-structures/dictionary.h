
#pragma once


namespace sorth::internal
{


    // In what context should this word be executed?
    enum class ExecutionContext
    {
        // Execute this word at compile time.
        compile_time,

        // Execute this word at run time.
        run_time
    };


    // What type of word is this?
    enum class WordType
    {
        // A word that is defined in Forth script.
        scripted,

        // A word that is defined in the interpreter.
        internal
    };


    // Should this word be visible in the dictionary?
    enum class WordVisibility
    {
        // The word is visible in the dictionary.
        visible,

        // The word is hidden from the dictionary.
        hidden
    };


    // How should this word's variables be managed?
    enum class WordContextManagement
    {
        // The word's variables are managed by the interpreter.
        managed,

        // The word's variables are managed by the word itself.
        unmanaged
    };


    struct Word
    {
        // Should this word be executed at compile or at run-time?
        ExecutionContext execution_context;

        // Was this word defined in Forth?
        WordType type;

        // Is this word hidden from the word dictionary?
        WordVisibility visibility;


        // Quick one line description of this word.
        std::string description;

        // How does this word interact with the stack?
        std::string signature;


        // Location this word was defined.
        Location location;

        // Index of the handler to execute.
        size_t handler_index;
    };


    // The Forth dictionary.  Handlers for Forth words are not stored directly in the dictionary.
    // Instead they are stored in their own list and the index and any important flags are what is
    // stored in the dictionary directly.
    //
    // Also note that the dictionary is implemented as a stack of dictionaries.  This allows the
    // Forth to contain scopes.  If a word is redefined in a higher scope, it effectively replaces
    // that word until that scope is released.
    class Dictionary
    {
        private:
            using SubDictionary = std::unordered_map<std::string, Word>;
            using DictionaryStack = std::list<SubDictionary>;

            DictionaryStack stack;

        public:
            Dictionary();
            Dictionary(const Dictionary& dictionary);

        public:
            void insert(const std::string& text, const Word& value);
            std::tuple<bool, Word> find(const std::string& word) const;

        public:
            void mark_context();
            void release_context();

        public:
            std::map<std::string, Word> get_merged_dictionary() const;

        protected:
            friend std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);
    };


    std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary);


}


#include "sorth.h"



namespace sorth::internal
{


    std::vector<std::string> inverse_lookup_list(const Dictionary& dictionary,
                                                 const WordList& word_handlers)
    {
        std::vector<std::string> name_list;

        name_list.resize(word_handlers.size());

        for (auto iter = dictionary.stack.begin(); iter != dictionary.stack.end(); ++iter)
        {
            for (auto word_iter = iter->begin(); word_iter != iter->end(); ++word_iter)
            {
                name_list[word_iter->second.handler_index] = word_iter->first;
            }
        }

        return name_list;
    }


    std::ostream& operator <<(std::ostream& stream, const Dictionary& dictionary)
    {
        // First merge all the sub-dictionaries into one sorted dictionary.  Note that words will
        // appear only once.  Even if they are redefined at higher scopes.  Only the newest version
        // will be displayed.
        std::map<std::string, Word> new_dictionary = dictionary.get_merged_dictionary();

        int visible_count = 0;  // Keep track of the number of visible words.
        int max = 0;            // For formatting, find out the largest word width.

        for (const auto& word : new_dictionary)
        {
            if (!word.second.is_hidden)
            {
                ++visible_count;

                if (max < word.first.size())
                {
                    max = (int)word.first.size();
                }
            }
        }

        // Now, print out the consolidated dictionary.
        stream << visible_count << " words defined." << std::endl;

        for (const auto& word : new_dictionary)
        {
            if (!word.second.is_hidden)
            {
                stream << std::left << std::setw(max) << word.first << "  "
                       << std::right << std::setw(6) << word.second.handler_index;

                if (word.second.is_immediate)
                {
                    stream << "  immediate";
                }
                else
                {
                    stream << "           ";
                }

                if (word.second.description)
                {
                    stream << "  --  " << (*word.second.description);
                }

                stream << std::endl;
            }
        }

        return stream;
    }


    Dictionary::Dictionary()
    {
        // Start with an empty dictionary.
        mark_context();
    }


    Dictionary::Dictionary(const Dictionary& dictionary)
    {
        // start with the empty dictionary.
        mark_context();

        // Now merge all the sub dictionaries into this one.
        for (auto iter = dictionary.stack.rbegin(); iter != dictionary.stack.rend(); ++iter)
        {
            stack.front().insert(iter->begin(), iter->end());
        }
    }


    void Dictionary::insert(const std::string& text, const Word& value)
    {
        auto& current_dictionary = stack.front();
        auto iter = current_dictionary.find(text);

        if (iter != current_dictionary.end())
        {
            iter->second = value;
        }
        else
        {
            current_dictionary.insert(SubDictionary::value_type(text, value));
        }
    }


    std::tuple<bool, Word> Dictionary::find(const std::string& word) const
    {
        for (auto stack_iter = stack.begin(); stack_iter != stack.end(); ++stack_iter)
        {
            auto iter = stack_iter->find(word);

            if (iter != stack_iter->end())
            {
                return { true, iter->second };
            }
        }

        return { false, {} };
    }


    void Dictionary::mark_context()
    {
        // Create a new dictionary.
        stack.push_front({});
    }


    void Dictionary::release_context()
    {
        stack.pop_front();

        // There should always be at least one dictionary.  If there isn't something has
        // gone horribly wrong.
        assert(!stack.empty());
    }


    std::map<std::string, Word> Dictionary::get_merged_dictionary() const
    {
        std::map<std::string, Word> new_dictionary;

        for (const auto& sub_dictionary : stack)
        {
            for (const auto& word_iter : sub_dictionary)
            {
                new_dictionary.insert(word_iter);
            }
        }

        return new_dictionary;
    }


}

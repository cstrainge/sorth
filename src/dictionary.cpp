
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
        // will be displayed.  (Unless if the newer version is hidden and the older isn't.)
        std::map<std::string, Word> new_dictionary;

        for (const auto& sub_dictionary : dictionary.stack)
        {
            for (const auto& word_iter : sub_dictionary)
            {
                if (!word_iter.second.is_hidden)
                {
                    new_dictionary.insert(word_iter);
                }
            }
        }

        // For formatting, find out the largest word width.
        size_t max = 0;

        for (const auto& word : new_dictionary)
        {
            if (max < word.first.size())
            {
                max = word.first.size();
            }
        }

        // Now, print out the consolidated dictionary.

        stream << new_dictionary.size() << " words defined." << std::endl;

        for (const auto& word : new_dictionary)
        {
            stream << std::setw(max) << word.first << " "
                      << std::setw(6) << word.second.handler_index;

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

        return stream;
    }


    Dictionary::Dictionary()
    {
        // Start with an empty dictionary.
        mark_context();
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


}

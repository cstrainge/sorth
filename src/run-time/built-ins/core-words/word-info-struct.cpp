
#include "sorth.h"



namespace sorth::internal
{


    namespace
    {


        DataObjectDefinitionPtr make_location_info_definition()
        {
            auto location_definition = std::make_shared<DataObjectDefinition>();

            location_definition->name = "sorth.location";
            location_definition->fieldNames.push_back("path");
            location_definition->fieldNames.push_back("line");
            location_definition->fieldNames.push_back("column");

            return location_definition;
        }


        DataObjectDefinitionPtr make_word_info_definition()
        {
            auto word_definition = std::make_shared<DataObjectDefinition>();

            word_definition->name = "sorth.word";
            word_definition->fieldNames.push_back("name");
            word_definition->fieldNames.push_back("is_immediate");
            word_definition->fieldNames.push_back("is_scripted");
            word_definition->fieldNames.push_back("description");
            word_definition->fieldNames.push_back("signature");
            word_definition->fieldNames.push_back("handler_index");
            word_definition->fieldNames.push_back("location");

            return word_definition;
        }


        DataObjectDefinitionPtr location_definition = make_location_info_definition();
        DataObjectDefinitionPtr word_info_definition = make_word_info_definition();


    }


    DataObjectPtr make_word_info_instance(InterpreterPtr& interpreter,
                                          std::string& name,
                                          Word& word)
    {
        auto new_location = make_data_object(location_definition);
        new_location->fields[0] = word.location.get_path();
        new_location->fields[1] = (int64_t)word.location.get_line();
        new_location->fields[2] = (int64_t)word.location.get_column();

        auto new_word = make_data_object(word_info_definition);
        new_word->fields[0] = name;
        new_word->fields[1] = word.is_immediate;
        new_word->fields[2] = word.is_scripted;
        new_word->fields[3] = word.description ? *word.description : "";
        new_word->fields[4] = word.signature ? *word.signature : "";
        new_word->fields[5] = (int64_t)word.handler_index;
        new_word->fields[6] = new_location;

        return new_word;
    }


    void register_word_info_struct(const Location& location, InterpreterPtr& interpreter)
    {
        create_data_definition_words(location, interpreter, location_definition, true);
        create_data_definition_words(location, interpreter, word_info_definition, true);
    }


}

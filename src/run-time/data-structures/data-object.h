
#pragma once


namespace sorth
{


    using ValueList = std::vector<Value>;


    // The base definition of a data object, useful for reflection and creation of the actual data
    // objects.
    struct DataObjectDefinition
    {
        using NameList = std::vector<std::string>;

        std::string name;     // The name of the type.

        bool is_hidden;       // Is the structure and it's words hidden from the word list?

        NameList fieldNames;  // Names of all the fields.
        ValueList defaults;   // The default values of these fields.
    };


    using DataObjectDefinitionPtr = std::shared_ptr<DataObjectDefinition>;
    using DefinitionList = internal::ContextualList<DataObjectDefinitionPtr>;


    struct DataObject
    {
        DataObjectDefinitionPtr definition;  // Reference of the base definition.
        ValueList fields;                    // The actual values of the structure.

        public:
            size_t hash() const noexcept;
    };


    // Create and register a new data definition.
    DataObjectDefinitionPtr create_data_definition(InterpreterPtr& interpreter,
                                                   std::string name,
                                                   ArrayPtr fields,
                                                   ArrayPtr defaults,
                                                   internal::WordVisibility visibility);

    void create_data_definition_words(const internal::Location &location,
                                      InterpreterPtr &interpreter,
                                      DataObjectDefinitionPtr &definition_ptr,
                                      internal::WordVisibility visibility);

    // Create a new data object for the given definition.
    DataObjectPtr make_data_object(const DataObjectDefinitionPtr& definition_ptr);


    // When we print out a data structure we include the definition so that we can include field
    // names along with the name of the type itself.
    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data);


    std::strong_ordering operator <=>(const DataObject& lhs, const DataObject& rhs);

    std::strong_ordering operator <=>(const DataObjectPtr& lhs, const DataObjectPtr& rhs);


    inline bool operator ==(const DataObject& lhs, const DataObject& rhs)
    {
        return (lhs <=> rhs) == std::strong_ordering::equal;
    }

    inline bool operator !=(const DataObject& lhs, const DataObject& rhs)
    {
        return (lhs <=> rhs) != std::strong_ordering::equal;
    }

    inline bool operator ==(const DataObjectPtr& lhs, const DataObjectPtr& rhs)
    {
        return *lhs == *rhs;
    }

    inline bool operator !=(const DataObjectPtr& lhs, const DataObjectPtr& rhs)
    {
        return *lhs == *rhs;
    }


}

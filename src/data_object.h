
#pragma once


namespace sorth
{


    // The base definition of a data object, useful for reflection and creation of the actual data
    // objects.

    struct DataObjectDefinition
    {
        using NameList = std::vector<std::string>;

        std::string name;     // The name of the type.
        NameList fieldNames;  // Names of all the fields.
        ValueList defaults;   // The default values of these fields.
    };


    using DataObjectDefinitionPtr = std::shared_ptr<DataObjectDefinition>;
    using DefinitionList = internal::ContextualList<DataObjectDefinitionPtr>;


    struct DataObject
    {
        DataObjectDefinitionPtr definition;  // Reference of the base definition.
        ValueList fields;                    // The actual values of the structure.
    };


    // When we print out a data structure we include the definition so that we can include field
    // names along with the name of the type itself.
    std::ostream& operator <<(std::ostream& stream, const DataObjectPtr& data);


    bool operator ==(const DataObjectPtr& rhs, const DataObjectPtr& lhs);


}

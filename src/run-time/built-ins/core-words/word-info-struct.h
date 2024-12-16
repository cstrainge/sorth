
#pragma once



namespace sorth::internal
{


    DataObjectPtr make_word_info_instance(InterpreterPtr& interpreter,
                                          std::string& name,
                                          Word& word);


    void register_word_info_struct(const Location& location, InterpreterPtr& interpreter);


}

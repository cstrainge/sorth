
#pragma once


namespace sorth::internal
{


    // The contextual list allows the interpreter to keep track of various contexts or scopes.
    // Variables and other forms of data are kept track of by indexable lists.  The idea being that
    // the list can expand as new items are added.  We extend this concept with contexts or scopes
    // that when a given context is exited, all values in that context are forgotten.

    template <typename value_type>
    class ContextualList
    {
        private:
            struct SubList
            {
                std::vector<value_type> items;
                size_t start_index;
            };

            using ListStack = std::list<SubList>;

            ListStack stack;

        public:
            ContextualList()
            {
                // Make sure we have an empty list ready to be populated.
                mark_context();
            }

            ContextualList(const ContextualList& list)
            {
                // Crate a empty list.
                mark_context();

                // Merge the contexts from the source list into a single context here.
                for (auto iter = list.stack.rbegin(); iter != list.stack.rend(); ++iter)
                {
                    stack.front().items.insert(stack.front().items.end(),
                                               iter->items.begin(),
                                               iter->items.end());
                }
            }

        public:
            size_t size() const
            {
                return stack.front().start_index + stack.front().items.size();
            }

            size_t insert(const value_type& value)
            {
                stack.front().items.push_back(value);
                return size() - 1;
            }

            value_type& operator [](size_t index)
            {
                if (index >= size())
                {
                    throw_error("Index out of range.");
                }

                for (auto stack_iter = stack.begin(); stack_iter != stack.end(); ++stack_iter)
                {
                    if (index >= stack_iter->start_index)
                    {
                        index -= stack_iter->start_index;
                        return stack_iter->items[index];
                    }
                }

                throw_error("Index not found.");
            }

        public:
            void mark_context()
            {
                size_t start_index = 0;

                if (!stack.empty())
                {
                    start_index = stack.front().start_index + stack.front().items.size();
                }

                stack.push_front({ .items = {}, .start_index = start_index });
            }

            void release_context()
            {
                stack.pop_front();
            }
    };


}

#pragma once

#include "egolib/platform.h"

namespace Ego
{
    namespace Script
    {
        using namespace std;

        /**
         * @brief
         *  A descriptor for enumerations.
         * @details
         *  Maps strings to enumeration elements.
         * @todo
         *  Do we meet the requirements for range-based for loops?
         * @todo
         *  The "name" stored in the enum descriptor should be in fact a "qualified name".
         * @author
         *  Michael Heilmann
         * @param EnumType
         *  an enumeration type
         */
        template <typename EnumType>
        struct EnumDescriptor
        {
            static_assert(is_enum<EnumType>::value, "not an enumeration type");
        private:
            /**
             * @brief
             *  The type of a map from names to values.
             * @remark
             *  This also ensures that no two enumeration elements of the same name can exist
             *  just like in C++. However, it is very-well possible the two different names
             *  map to the same value just like in C++.
             */
            map<string,EnumType> _elements;
            /**
             * @brief
             *  The name of the enumeration.
             */
            string _name;
        public:

            // Support for range-based for loops (not required).
            typedef typename map<string, EnumType>::iterator iterator;
            
            // Support for range-based for loops (not required).
            typedef typename map<string, EnumType>::const_iterator const_iterator;

            // Support for range-based for loops.
            const_iterator begin() const
            {
                return _elements.begin();
            }
            const_iterator cbegin() const
            {
                return _elements.cbegin();
            }
            const_iterator end() const
            {
                return _elements.end();
            }
            const_iterator cend() const
            {
                return _elements.cend();
            }

        public:


        #ifdef _MSC_VER
            EnumDescriptor(const string& name, const initializer_list<pair<const string, EnumType>>& list) :
                _name(name), _elements(list)
            {}
        #else
            EnumDescriptor(const string& name, const initializer_list<pair<const string, EnumType>>& list) :
                 _name(name), _elements{ list }
            {}
        #endif

            EnumDescriptor(const string& name) :
                _name(), _elements()
            {}

            const string& getName() const
            {
                return _name;
            }

            void set(const string& name, EnumType value)
            {
                _elements[name] = value;
            }

            /**
             * @brief
             *  Map from an enumeration element value to an enumeration element name.
             * @param value
             *  the value to search a name for
             * @return
             *  an iterator referring to the first entry with a value equal to @a value
             * @remark
             *  As the mapping from values to names is not necessarily unique,
             *  get(EnumType,Iterator) exists which allows to start searching
             *  at a distinct and thus diescover eventually aliases.
             *  @code
             *  auto x = d.get(8);
             *  if (x != d.end())
             *  {
             *      auto y = x;
             *      ++y;
             *      y = d.get(8,y);
             *      if (y != d.end())
             *      {
             *          //if (x->second != y->second || x->second != 8)
             *          //{
             *          //  throw logic_error("impossible!);
             *          //}
             *          cout << "enumeration elements " << x->first << " and " << y->first << " map to the same value " << x->second << endl;
             *      }
             *  }
             *  @endcode
             *
             */
            const_iterator find(EnumType value) const
            {
                return find(value, _elements.cbegin());
            }
            /**
             * @brief
             *  Map from an enumeration element value to an enumeration element name.
             * @param value
             *  the value to search a name for
             * @param start
             *  an iterator at which the seach shall start at
             * @return
             *  an iterator referring to the first entry with a value equal to @a value or <tt>cend()</tt>
             */
            const_iterator find(EnumType value, const_iterator start) const
            {
                for (auto it = start; it != _elements.cend(); ++it)
                {
                    if ((*it).second == value)
                    {
                        return it;
                    }
                }
                return _elements.cend();
            }

            /**
             * @breif
             *  Find the enumeration element value for an enumeration element name.
             * @param name
             *  the enumeration element name
             * @return
             *  an iterator referring to the first entry with a name equal to @a name or <tt>cend()</tt>
             */
            const_iterator find(const string& name)
            {
                return _elements.find(name);
            }

        };
    }
}

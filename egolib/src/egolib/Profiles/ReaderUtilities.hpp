#pragma once

#include "egolib/platform.h"

using namespace std;

/**
 * @brief
 *  A reader for enumerations.
 * @details
 *  Maps strings to enumeration elements.
 * @author
 *  Michael Heilmann
 */
template <typename EnumType>
struct EnumReader
{
public:
    typedef typename map<string, EnumType> Map;
public:

    struct Iterator
    {
    private:
        typename Map::iterator _it;

    public:

        Iterator() :
            _it()
        {}

        Iterator(typename Map::iterator& it) :
            _it(it)
        {}
        
        Iterator(typename Map::iterator&& it) :
        _it(it)
        {}

        Iterator(const Iterator& other) :
            _it(other._it)
        {}

        ~Iterator()
        {}

        Iterator& operator=(const Iterator& other)
        {
            _it = other._it;
        }

        bool operator==(const Iterator& other) const
        {
            return _it == other._it;
        }

        bool operator!=(const Iterator& other) const
        {
            return _it != other._it;
        }

        Iterator operator--(int) ///< postfix decrement operator
        {
            return _it--;
        }

        Iterator operator++(int) ///< postfix increment operator
        {
            return _it++;
        }

        Iterator& operator--() ///< prefix decrement operator
        {
            return --_it;
        }

        Iterator& operator++() ///< prefix increment operator
        {
            return ++_it;
        }

        EnumType& operator*() const
        {
            return (*_it).second;
        }

        EnumType *operator->()
        {
            return &((*_it).second);
        }

    };

private:
    Map _elements;
    string _name;
public:

#ifdef _MSC_VER
    EnumReader(const string& name, const initializer_list<std::pair<const string, EnumType>>& list) :
        _elements(list),
        _name(name)
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            const auto& p = *it;
            _elements[p.first] = p.second;
        }
    }
#else
    EnumReader(const string& name, const initializer_list<std::pair<const string, EnumType>>& list) :
         _elements{ list }, 
         _name(name)
    {
    }
#endif

    EnumReader() :
        _name(),
        _elements()
    {
    }

    const string& getName() const
    {
        return _name;
    }

    Iterator set(const string& name, EnumType value)
    {
        _elements[name] = value;
    }

    Iterator get(const string& name)
    {
        auto it = _elements.find(name);
        return Iterator(it);
    }

public:

    const EnumType& operator[](const string& name) const
    {
        return _elements[name];
    }

    EnumType& operator[](const string& name)
    {
        return _elements[name];
    }

    const Iterator begin() const
    {
        return Iterator(_elements.begin());
    }

    Iterator begin()
    {
        return Iterator(_elements.begin());
    }

    const Iterator end() const
    {
        return Iterator(_elements.end());
    }

    Iterator end()
    {
        return Iterator(_elements.end());
    }

};

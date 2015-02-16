#pragma once

/// @brief  A reader for enumerations.
/// @detail Maps strings to enumeration elements.
/// @author Michael Heilmann
template <typename EnumType>
struct EnumReader
{
public:

    struct Iterator
    {
    private:
        typename std::map<std::string, EnumType>::iterator _it;

    public:

        Iterator() :
            _it()
        {}

        Iterator(typename std::map<std::string, EnumType>::iterator& it) :
            _it(it)
        {}
        
        Iterator(typename std::map<std::string, EnumType>::iterator&& it) :
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
    std::map<std::string, EnumType> _elements;
    std::string _name;
public:

#ifdef _MSC_VER
    EnumReader(const std::string& name, const std::initializer_list<std::pair<const std::string, EnumType>>& list) :
        _name(name), _elements()
    {
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            const std::pair<const std::string, EnumType>& p = *it;
            _elements[p.first] = p.second;
        }
    }
#else
    EnumReader(const std::string& name, const std::initializer_list<std::pair<const std::string, EnumType>>& list) :
        _name(name), _elements{ list }
    {
    }
#endif

    EnumReader() :
        _elements()
    {
    }

    const std::string& getName() const
    {
        return _name;
    }

    Iterator set(const std::string& name, EnumType value)
    {
        _elements[name] = value;
    }

    Iterator get(const std::string& name)
    {
        auto it = _elements.find(name);
        return Iterator(it);
    }

public:

    const EnumType& operator[](const std::string& name) const
    {
        return _elements[name];
    }

    EnumType& operator[](const std::string& name)
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

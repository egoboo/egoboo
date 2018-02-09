#pragma once

#include "egolib/Configuration/Enum.hpp"
#include "egolib/Configuration/Number.hpp"
#include "egolib/Configuration/StringOrBool.hpp"
#include "egolib/FileFormats/ConfigFile/configfile.h"

namespace Ego {
namespace Configuration {
/// @brief Base of any configuration.
struct Configuration
{
protected:
    // This is utility code for iterating over an std::tuple.
    template<typename TupleTypeArg, typename FunctionTypeArg>
    void for_each(TupleTypeArg&&, FunctionTypeArg, std::integral_constant<size_t,
                  std::tuple_size<typename std::remove_reference<TupleTypeArg>::type >::value>)
    {}

    // This is utility code for iterating over an std::tuple.
    template<std::size_t I, typename TupleTypeArg, typename FunctionTypeArg,
        typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleTypeArg>::type>::value>::type >
        void for_each(TupleTypeArg&& t, FunctionTypeArg f, std::integral_constant<size_t, I>)
    {
        f(std::get<I>(t)); // Call the function.
        for_each(std::forward<TupleTypeArg>(t), f, std::integral_constant<size_t, I + 1>()); // Advance.
    }

    // This is utility code for iterating over an std::tuple.
    template<typename TupleType, typename FunctionType>
    void for_each(TupleType&& t, FunctionType f)
    {
        for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
    }

    struct Load
    {
    private:
        std::shared_ptr<ConfigFile> source;
    public:
        Load(const Load& other) :
            source(other.source)
        {}
        Load& operator=(const Load& other)
        {
            source = other.source;
            return *this;
        }
        Load(const std::shared_ptr<ConfigFile> &source) :
            source(source)
        {}
        template<typename VariableTy>
        void operator()(VariableTy& variable) const
        {
            std::string valueString;
            if (!source->get(variable.getName(), valueString))
            {
                variable.setValue(variable.getDefaultValue());
            }
            else
            {
                variable.decodeValue(valueString);
            }
        }
    };

    struct Store
    {
    private:
        std::shared_ptr<ConfigFile> target;
    public:
        Store(const Store& other) :
            target(other.target)
        {}
        Store(const std::shared_ptr<ConfigFile> &target) :
            target(target)
        {}
        Store& operator=(const Store& other)
        {
            target = other.target;
            return *this;
        }
        template<typename Variable>
        void operator()(Variable& variable) const
        {
            std::string value;
            variable.encodeValue(value);
            target->set(variable.getName(), value);
        }
    };

};

} // namespace Configuration
} // namespace Ego

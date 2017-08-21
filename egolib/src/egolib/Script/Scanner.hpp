#pragma once

#include "egolib/Script/Traits.hpp"
#include "idlib/parsing_expressions.hpp"
#include "egolib/Script/TextInputFile.hpp"

#pragma push_macro("ERROR")
#undef ERROR

namespace Ego { namespace Script {

/// @brief A scanner.
/// @tparam TraitsArg the type of the traits
template <typename TraitsArg = Traits<char>>
struct Scanner
{
private:
    /// @brief The line number.
    size_t m_line_number;

    //// @brief The file name.
    std::string m_file_name;

    struct transform_functor;

public:
    using Traits = TraitsArg;
    using SymbolType = typename Traits::Type;
    using ExtendedSymbolType = typename Traits::ExtendedType;
	using iterator_type = id::transform_iterator<transform_functor, typename std::vector<char>::const_iterator>;
    
private:
	struct transform_functor
	{
		using source_type = typename std::iterator_traits<typename std::vector<char>::const_iterator>::reference;
		using target_type = ExtendedSymbolType;
		target_type operator()(source_type x) const
		{
			return x;
		}
	};
	/// @brief The lexeme accumulation buffer.
    std::vector<char> m_buffer;
    /// @brief The input buffer.
    std::vector<char> m_input_buffer;
    /// @brief Range wrapping the iterators pointing to the beginning and the end of the list of input symbols.
	id::iterator_range<iterator_type> m_range;
	/// @brief Iterator pointing to the current input symbols in the list of input symbols.
	iterator_type m_current;

protected:
    /// @brief Construct this scanner.
    /// @param file_name the filename
    /// @throw id::runtime_error the file can not be read
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    Scanner(const std::string& file_name) :
        m_file_name(file_name), m_line_number(1), m_input_buffer(),
        m_buffer(), m_current(), m_range()
    {
        vfs_readEntireFile
        (
            file_name,
            [this](size_t number_of_bytes, const char *bytes)
            {
                m_input_buffer.insert(m_input_buffer.end(), bytes, bytes + number_of_bytes);
            }
        );
        //
		auto begin = iterator_type(m_input_buffer.cbegin(), transform_functor{});
		auto end = iterator_type(m_input_buffer.cend(), transform_functor{});
		auto current = iterator_type(m_input_buffer.cbegin(), transform_functor{});
		m_range = id::make_iterator_range(begin, end);
		m_current = current;
    }

    /// @brief Set the input.
    /// @param file_name the filename
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    /// If an exception is raised, the scanner retains its state.
    void set_input(const std::string& file_name)
    {
        std::vector<char> temporary_input_buffer;
        std::string temporary_file_name = file_name;
        // If this succeeds, then we're set.
        vfs_readEntireFile(file_name, [&temporary_input_buffer](size_t number_of_bytes, const char *bytes)
        {
            temporary_input_buffer.insert(temporary_input_buffer.end(), bytes, bytes + number_of_bytes);
        });
        m_line_number = 1;
        m_file_name.swap(temporary_file_name);
        m_input_buffer.swap(temporary_input_buffer);
        //
		auto begin = iterator_type(m_input_buffer.cbegin(), transform_functor{});
		auto end = iterator_type(m_input_buffer.cend(), transform_functor{});
		auto current = iterator_type(m_input_buffer.cbegin(), transform_functor{});
		m_range = id::make_iterator_range(begin, end);
		m_current = current;
    }

    /// @brief Destruct this scanner.
    virtual ~Scanner()
    {}

public:
    /// @brief Get the file name.
    /// @return the file name
    const std::string& get_file_name() const
    {
        return m_file_name;
    }

    /// @brief Get the line number.
    /// @return the line number
    size_t get_line_number() const
    {
        return m_line_number;
    }

    /// @brief Get the location.
    /// @return the location
    id::location get_location() const
    {
        return id::location(get_file_name(), get_line_number());
    }

    /// @brief Get the lexeme text.
    /// @return the lexeme text
    std::string get_lexeme_text() const
    {
        return std::string(m_buffer.cbegin(), m_buffer.cend());
    }

    /// @brief Clear the lexeme text.
    void clear_lexeme_text()
    {
        m_buffer.clear();
    }

public:
    /// @brief Get the current input symbol.
    /// @return the current input symbol
    ExtendedSymbolType current() const
    {
        return *m_current;
    }

public:
    /// @brief Advance to the next input symbol.
    void next()
    {
        m_current++;
    }

    /// @brief Write the specified symbol.
    /// @param symbol the symbol
    inline void write(const ExtendedSymbolType& symbol)
    {
        assert(!Traits::is_pua_bmp(symbol) && !Traits::is_zt(symbol));
        m_buffer.push_back(static_cast<SymbolType>(symbol));
    }

    /// @brief Write the specified symbol and advance to the next symbol.
    /// @param symbol the symbol
    inline void write_and_next(const ExtendedSymbolType& symbol)
    {
        write(symbol);
        next();
    }

    /// @brief Save the current extended character.
    inline void save()
    {
        write(current());
    }

    /// @brief Save the current input symbol and advance to the next input symbol.
    inline void save_and_next()
    {
        save();
        next();
    }

public:
    /// @brief Get if the current symbol equals another symbol.
    /// @param symbol the other extended character
    /// @return @a true if the current symbol equals the other symbol, @a false otherwise
    inline bool is(const ExtendedSymbolType& symbol) const
    {
		if (m_current == m_range.end())
		{
			return false;
		}
        return symbol == *m_current;
    }

private:
	static decltype(auto) make_sym(char c)
	{
		return id::parsing_expressions::sym<ExtendedSymbolType>(c);
	}

public:
	static decltype(auto) TILDE()
	{ return make_sym('~'); }

	static decltype(auto) PLUS()
	{ return make_sym('+'); }

	static decltype(auto) MINUS()
	{ return make_sym('-'); }

	static decltype(auto) LEFT_SQUARE_BRACKET()
	{ return make_sym('['); }

	static decltype(auto) RIGHT_SQUARE_BRACKET()
	{ return make_sym(']'); }

	static decltype(auto) UNDERSCORE()
	{ return make_sym('_'); }

	static decltype(auto) EXCLAMATION_MARK()
	{ return make_sym('!'); }

	static decltype(auto) QUESTION_MARK()
	{ return make_sym('?'); }

	static decltype(auto) EQUAL()
	{ return make_sym('='); }

	static decltype(auto) SINGLE_QUOTE()
	{ return make_sym('\''); }

	static decltype(auto) BACKSLASH()
	{ return make_sym('\\'); }

	static decltype(auto) DOLLAR()
	{ return make_sym('$'); }

	static decltype(auto) SLASH()
	{ return make_sym('/'); }

    static decltype(auto) WHITE_SPACE()
    {
        static const auto p = id::parsing_expressions::whitespace<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) NEW_LINE()
    {
        static const auto p = id::parsing_expressions::newline<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) ALPHA()
    {
        static const auto p = id::parsing_expressions::alpha<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) DIGIT()
    {
        static const auto p = id::parsing_expressions::digit<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) END_OF_INPUT()
    {
        static const auto p = id::parsing_expressions::end_of_input<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) ERROR()
    { return make_sym(Traits::error()); }

    template <typename T>
    bool ise(const T& e) const
    {
        return (bool)e(m_current, m_range.end());
    }

public:
    /// @code
    /// new_line := NEW_LINE?
    /// @endcode
    void new_line(std::function<void(char)> action)
    {
        if (ise(NEW_LINE()))
        {
            auto old = current();
            if (action)
            { 
                action('\n');
            }
            next();
            if (ise(NEW_LINE()) && old != current())
            {
                next();
            }
            m_line_number++;
        }
    }

    /// @code
    /// new_lines = NEW_LINE*
    /// @endcode
    void new_lines(std::function<void(char)> action)
    {
        while (ise(NEW_LINE()))
        {
            auto old = current();
            if (action)
            {
                action('\n');
            }
            next();
            if (ise(NEW_LINE()) && old != current())
            {
                next();
            }
            m_line_number++;
        }
    }

};

} } // namespace Ego::Script

#pragma pop_macro("ERROR")

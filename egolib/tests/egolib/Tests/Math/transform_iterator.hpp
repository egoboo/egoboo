#pragma once

namespace id {

/// @brief A transform iterator.
/// @tparam Iterator the underlying iterator type
/// @tparam UnaryFunction the unary function type
template <typename Iterator, typename UnaryFunction>
class transform_iterator
{
public:
	/// @brief The reference type.
	/// This type is defined as
	/// @code
	/// std::result_of<const UnaryFunction(std::iterator_traits<Iterator>::reference)>::type
	/// @endcode
	using reference = std::result_of<const UnaryFunction(std::iterator_traits<Iterator>::reference)>::type;

	/// @brief The value type.
	/// This type is defined as
	/// @code
	/// std::remove_cv<std::remove_reference<reference> >::type
	/// @endcode
	using value = std::remove_cv<std::remove_reference<reference> >::type;

	/// @brief The category of this iterator.
	/// This type is defined as
	/// @code
	/// std::iterator_traits<Iterator>::iterator_category
	/// @endcode
	using category = std::iterator_traits<Iterator>::iterator_category;

private:
	Iterator m_iterator;
	UnaryFunction m_unary_function;

public:
	transform_iterator(Iterator iterator, UnaryFunction unary_function);

public:
	/// @brief Default construct this iterator.
	transform_iterator();

	/// @brief Copy construct this iterator.
	/// @param other the other iterator
	transform_iterator(const transform_iterator<Iterator, Function>& other) = default;

	/// @brief Copy assign this iterator.
	/// @param other the other iterator
	/// @return this iterator
	transform_iterator<Iterator, Function>& operator=(const transform_iterator<Iterator, Function>& other) = default;
	
	transform_iterator<Iterator, UnaryFunction>& operator--() {
		--m_iterator;
		return *this;
	}
		
	transform_iterator<Iterator, UnaryFunction> operator--(int) {
		auto temporary = *this;
		--(*this);
		return temporary;	
	}
	
	transform_iterator<Iterator, UnaryFunction>& operator++() {
		++m_iterator;
		return *this;
	}

	transform_iterator<Iterator, UnaryFunction> operator++(int) 	{
		auto temporary = *this;
		++(*this);
		return temporary;
	}
};

/*-----------------------------------------------------------------------------------------------*/

template <typename Iterator, typename UnaryFunction>
transform_iterator::transform_iterator(Iterator iterator, UnaryFunction unary_function) :
	m_iterator(iterator),
	m_unary_function(unary_function)
{}

/*-----------------------------------------------------------------------------------------------*/

template <typename Iterator, typename UnaryFunction>
transform_iterator::transform_iterator() :
	m_iterator(),
	m_unary_function()
{}

/*-----------------------------------------------------------------------------------------------*/

template <typename Iterator, typename UnaryFunction>
transform_iterator<Iterator, UnaryFunction> make_transform_iterator(Iterator iterator, UnaryFunction unary_function)
{
	transform_iterator<Iterator, UnaryFunction>(iterator, unary_function);
}

} // namespace id

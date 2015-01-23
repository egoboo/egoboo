
/// @file    egolib/math/DynamicVector.hpp
/// @brief   Vectors with compile-time dimensionality.
/// @details The efficiency of this template depends on the optimization capabilities of your compiler (in particular, loop unrolling).
/// @todo    Rename to <tt>egolib/math/CTVector.hpp</tt>.
/// @todo    Rename to <tt>Ego::Math::CTVector</tt>.
#pragma once

namespace Ego {
	namespace Math {
		template <typename Type, size_t Dimensionality>
		struct Vector {
			static_assert(Dimensionality > 0);
			private Type components[Dimensionality];
			
			Vector()
			{

			}
			Vector(const Vector<Type, Dimensionality>& other)
			{
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					components = other.components;
				}
			}
			Type dot(const Vector<Type,Dimensionality>& other) const
			{
				Type dot();
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					dot += components[i] * other.components[i];
				}
				return dot;
			}
			bool equal(const Vector<Type, Dimensionality>& other)
			{
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					if (components[i] != other.components[i])
					{
						return false;
					}
				}
				return true;
			}
			bool operator==(const Vector<Type, Dimensionality>& other) const
			{
				return equal(other);
			}
			bool operator!=(const Vector<Type, Dimensionality>& other) const
			{
				return !equal(other);
			}
			void assign(const Vector<Type, Dimensionality>& other)
			{
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					components = other.components;
				}
			}
			Vector<Type, Dimensionality>& operator=(const Vector<Type, Dimensionality>& other)
			{
				assign(other);
				return *this;
			}
			void add(const Vector<Type, Dimensionality>& other)
			{
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					components[i] += other.components[i];
				}
			}
			Vector<Type, Dimensionality> operator+=(const Vector<Type, Dimensionality>& other)
			{
				add(other);
				return *this;
			}
			Vector<Type, Dimensionality> operator+(const Vector<Type, Dimensionality>& other) const
			{
				Vector<Type, Dimensionality> result(*this);
				result.add(other);
				return result;
			}
			void sub(const Vector<Type, Dimensionality>& other)
			{
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					components[i] -= other.components[i];
				}
			}
			Vector<Type, Dimensionality> operator-=(const Vector<Type, Dimensionality>& other)
			{
				sub(other);
				return *this;
			}
			Vector<Type, Dimensionality> operator-(const Vector<Type, Dimensionality>& other) const
			{
				Vector<Type, Dimensionality> result(*this);
				result.sub(other);
				return result;
			}
			void mul(const Type& scalar) {
				for (size_t i = 0; i < Dimensionality; ++i)
				{
					components[i] *= scalar;
				}
			}
			Vector<Type, Dimensionality>& operator*=(const Type& scalar)
			{
				mul(scalar);
				return *this;
			}
			Vector<Type, Dimensionality> operator*(const Type& scalar) const
			{
				Vector<Type, Dimensionality> result(*this);
				result.mul(scalar);
				return result;
			}
			Type& operator()(size_t index)
			{
				return components[index];
			}
			const Type& operator()(size_t index) const
			{
				return components[index];
			}
		};
	};
};
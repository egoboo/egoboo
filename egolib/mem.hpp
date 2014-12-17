#pragma once

template<T> EgobooNew()
{
	return EgobooNew<T>(1);
}

template<T> EgobooNew(size_t length)
{
	return calloc(length, sizeof(T));
}
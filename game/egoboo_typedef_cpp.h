#pragma once

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.h should only be included is you are compling as c++
#endif

#include <exception>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// fix for the fact that assert is technically not supported in c++
class egoboo_exception : public std::exception
{
    protected:
        const char * what;
    public:
        egoboo_exception( const char * str ) : what( str ) {};
};

#define CPP_EGOBOO_ASSERT(X) if( !(X) ) { throw (std::exception*)(new egoboo_exception( #X )); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// definition of the reference template

template <typename _ty> class t_reference
{
    protected:
        REF_T    ref;
        _ty    * ptr;          // unused, needed to allow for template specilization?

    public:

        explicit t_reference( REF_T v = 0xFFFF, _ty * p = NULL ) : ref( v ), ptr( p ) {};

        t_reference<_ty> & operator = ( REF_T rhs )
        {
            ref = rhs;
            ptr = NULL;
            return *this;
        }

        const REF_T get_value() const { return ref; }

        friend bool operator == ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref == rhs; }
        friend bool operator != ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref != rhs; }
        friend bool operator >= ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref >= rhs; }
        friend bool operator <= ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref <= rhs; }
        friend bool operator < ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref <  rhs; }
        friend bool operator > ( const t_reference<_ty> & lhs, REF_T rhs )  { return lhs.ref >  rhs; }

        friend bool operator == ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs == rhs.ref; }
        friend bool operator != ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs != rhs.ref; }
        friend bool operator >= ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs >= rhs.ref; }
        friend bool operator <= ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs <= rhs.ref; }
        friend bool operator < ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs <  rhs.ref; }
        friend bool operator > ( REF_T lhs, const t_reference<_ty> & rhs )  { return lhs >  rhs.ref; }

        friend bool operator == ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref == rhs.ref; }
        friend bool operator != ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref != rhs.ref; }
        friend bool operator >= ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref >= rhs.ref; }
        friend bool operator <= ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref <= rhs.ref; }
        friend bool operator < ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref <  rhs.ref; }
        friend bool operator > ( const t_reference<_ty> & lhs, const t_reference<_ty> & rhs )  { return lhs.ref >  rhs.ref; }

        t_reference<_ty> & operator ++ ( int ) { ref++; return *this; }
        t_reference<_ty> & operator ++ ()      { ++ref; return *this; }

        t_reference<_ty> & operator -- ( int ) { if ( 0 == ref ) CPP_EGOBOO_ASSERT( NULL == "t_reference()::operator -- underflow" ); ref--; return *this; }
        t_reference<_ty> & operator -- ()      { if ( 0 == ref ) CPP_EGOBOO_ASSERT( NULL == "t_reference()::operator -- underflow" ); --ref; return *this; }
};

#define CPP_DECLARE_REF( TYPE, NAME ) typedef t_reference<TYPE> NAME

#define REF_TO_INT( X ) ((X).get_value())

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple array template

template < typename _ty, size_t _sz >
struct t_cpp_ary
{
private:
    _ty    lst[_sz];

public:
    _ty & operator []( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_cpp_ary::operator[] - index out of range" ); */ return lst[val]; }
    _ty * operator + ( const t_reference<_ty> & ref ) { const REF_T val = ref.get_value(); /* if ( val > _sz ) CPP_EGOBOO_ASSERT( NULL == "t_cpp_ary::operator + - index out of range" ); */ return lst + val; }
};

#define CPP_DECLARE_T_ARY(TYPE, NAME, COUNT) t_cpp_ary<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple list template that tracks free elements

template < typename _ty, size_t _sz >
struct t_cpp_list
{
    int    used_count;
    int    free_count;
    size_t used_ref[_sz];
    size_t free_ref[_sz];

    CPP_DECLARE_T_ARY( _ty, lst, _sz );

    t_cpp_list() { used_count = free_count = 0; }
};

#define CPP_DECLARE_LIST_EXTERN(TYPE, NAME, COUNT)    extern t_cpp_list<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_LIST_STATIC(TYPE,NAME, COUNT) static t_cpp_list<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_LIST(ACCESS,TYPE,NAME, COUNT) ACCESS t_cpp_list<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// a simple stack template

template < typename _ty, size_t _sz >
struct t_cpp_stack
{
    int   count;
    CPP_DECLARE_T_ARY( _ty, lst, _sz );

    t_cpp_stack() { count = 0; }
};

#define CPP_DECLARE_STACK_EXTERN(TYPE, NAME, COUNT)      extern t_cpp_stack<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_STACK_STATIC(TYPE, NAME, COUNT)  static t_cpp_stack<TYPE, COUNT> NAME
#define CPP_INSTANTIATE_STACK(ACCESS, TYPE, NAME, COUNT) ACCESS t_cpp_stack<TYPE, COUNT> NAME

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define egoboo_typedef_cpp_h

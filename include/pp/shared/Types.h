#pragma once

#include <pp/Common.h>

#include <cstdint>
#include <memory>

using byte = uint8_t;

using s16 = int16_t;
using u16 = uint16_t;

using s32 = int32_t;
using u32 = uint32_t;

using s64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// MariaDB needs this to compile. Defining it here avoids modification of their
// header files and thus additional licensing work.
using uint = unsigned int;

// some c++14 things for unix
#ifndef _WIN32
namespace std
{
	// make_unique
	template<class T> struct _Unique_if {
		typedef unique_ptr<T> _Single_object;
	};

	template<class T> struct _Unique_if<T[]> {
		typedef unique_ptr<T[]> _Unknown_bound;
	};

	template<class T, size_t N> struct _Unique_if<T[N]> {
		typedef void _Known_bound;
	};

	template<class T, class... Args>
	typename _Unique_if<T>::_Single_object
		make_unique(Args&&... args)
	{
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template<class T>
	typename _Unique_if<T>::_Unknown_bound
		make_unique(size_t n)
	{
		typedef typename remove_extent<T>::type U;
		return unique_ptr<T>(new U[n]());
	}

	template<class T, class... Args>
	typename _Unique_if<T>::_Known_bound
		make_unique(Args&&...) = delete;

	// Type aliases
	template< class T >
	using remove_cv_t = typename remove_cv<T>::type;

	template< class T >
	using remove_const_t = typename remove_const<T>::type;

	template< class T >
	using remove_volatile_t = typename remove_volatile<T>::type;

	template< class T >
	using add_cv_t = typename add_cv<T>::type;

	template< class T >
	using add_const_t = typename add_const<T>::type;

	template< class T >
	using add_volatile_t = typename add_volatile<T>::type;


	template< class T >
	using remove_reference_t = typename remove_reference<T>::type;


	template< class T >
	using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

	template< class T >
	using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;


	template< class T >
	using remove_pointer_t = typename remove_pointer<T>::type;

	template< class T >
	using add_pointer_t = typename add_pointer<T>::type;


	template< class T >
	using make_signed_t = typename make_signed<T>::type;

	template< class T >
	using make_unsigned_t = typename make_unsigned<T>::type;


	template< class T >
	using remove_extent_t = typename remove_extent<T>::type;

	template< class T >
	using remove_all_extents_t = typename remove_all_extents<T>::type;


	template< class T >
	using decay_t = typename decay<T>::type;


	template< bool B, class T = void >
	using enable_if_t = typename enable_if<B, T>::type;

	template< bool B, class T, class F >
	using conditional_t = typename conditional<B, T, F>::type;

	template< class... T >
	using common_type_t = typename common_type<T...>::type;

	template< class T >
	using underlying_type_t = typename underlying_type<T>::type;

	template< class T >
	using result_of_t = typename result_of<T>::type;
}
#endif // __UNIX

template< class T >
using bare_type_t = std::remove_cv_t<std::remove_reference_t<T>>;

template< class T >
using remove_all_t = std::remove_cv_t<std::remove_pointer_t<std::decay_t<std::remove_reference_t<T>>>>;

template<class T>
typename std::make_signed_t<T> as_signed(T t)
{
	return std::make_signed_t<T>(t);
}

template<class T>
typename std::make_unsigned_t<T> as_unsigned(T t)
{
	return std::make_unsigned_t<T>(t);
}

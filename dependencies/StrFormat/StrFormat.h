// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

#include "printf.h"
#include "BasicAdaptor.h"

namespace ts_printf
{
	namespace _details
	{
		template<typename CharType>
		class CStringAdaptor : CBasicAdaptor
		{
		public:
			typedef std::basic_string<CharType> CString;

			CStringAdaptor()
			{
			}

			void Write(CharType Char, size_t Count)
			{
				m_Result.append(Count, Char);
			}

			void Write(const CharType* szString, size_t Length)
			{
				m_Result.append(szString, Length);
			}

			CString& GetResult()
			{
				return m_Result;
			}

		private:
			CString m_Result;
			CStringAdaptor &operator =(const CStringAdaptor &);
		};
	}
};

// Render from format.
template<class CFormat, typename... A>
auto StrFormat(const CFormat&& Format, A&& ... Args) ->
std::basic_string<typename CFormat::char_type>
{
	ts_printf::_details::CStringAdaptor<typename CFormat::char_type> Adaptor;

	Format.render(Adaptor, std::forward<A>(Args)...);
	return Adaptor.GetResult();
}

// Create format object & render for zero terminated string pointers
template<typename CharPointer, typename... A>
auto StrFormat(CharPointer&& CharPtr, A&& ... Args) ->
std::enable_if_t < !std::is_base_of < ts_printf::_details::SBaseFormatter, std::decay_t<CharPointer >> ::value,
decltype(StrFormat(ts_printf::Format(std::forward<CharPointer>(CharPtr)), std::forward<A>(Args)...))
>
{
	return StrFormat(ts_printf::Format(std::forward<CharPointer>(CharPtr)), std::forward<A>(Args)...);
}

// Create format object & render for string references
template<class CString, typename... A>
auto StrFormat(CString& FormatString, A && ... Args) ->
typename std::enable_if < !std::is_base_of < ts_printf::_details::SBaseFormatter, std::decay_t<CString >> ::value &&
!std::is_pod<std::remove_reference_t<CString>>::value,
decltype(StrFormat(ts_printf::Format(std::forward<CString>(FormatString)), std::forward<A>(Args)...))
> ::type
{
	return StrFormat(ts_printf::Format(std::forward<CString>(FormatString)), std::forward<A>(Args)...);
};

// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#include <functional>

#pragma once

namespace ts_printf
{
	namespace _details
	{
		template<typename CharType>
		class CStringRenderer
		{
			typedef SFormatDesc<CharType> FormatDesc;
			typedef CCharBuffer<CharType> CharBuffer;

			template<typename T>
			static void RenderString(CharBuffer& Output, const T* pData, size_t Length)
			{
				// No need to allocate any buffer, we already have the buffer right here!
				// TODO: implement

				CharType* pCur = Output.Allocate(Length);
				const CharType* end = pCur + Length;

				// Copy into buffer
				for(; pCur != end; pCur++, pData++)
				{
					*pCur = *pData;
				}

				Output.SetLength(Length);
			}

			// char pointer
			template<typename T>
			static void StringDispatcher(const T *const &r, CharBuffer &Output)
			{
				RenderString(Output, r, std::char_traits<T>::length(r));
			}


			// char array
			template<typename T, size_t N>
			static void StringDispatcher(const T(&p)[N], CharBuffer &Output)
			{
				// we don't need the trailing zero character
				RenderString(Output, p, std::char_traits<T>::length(p));
			}


			// For basic_string of all types
			template<class CString>
			static typename std::enable_if<!std::is_pod<typename std::remove_reference<CString>::type>::value>::type
				StringDispatcher(const CString& String, CharBuffer &Output)
			{
				RenderString(Output, String.c_str(), String.length());
			}


		protected:

			// Only enable this string-renderer template if one of the following holds
			template<typename StringType>
			static typename std::enable_if<

				// We have a pointer to an allowed char type.
				// (It is assumed, that it is a pointer to a zero terminated string. anyone providing a raw byte pointer for printing should specify so by using an unsigned type pointer.)
				(std::is_pointer<typename std::remove_reference<StringType>::type>::value && is_char_type<typename std::remove_cv<typename std::remove_pointer<typename std::decay<typename std::remove_reference<StringType>::type>::type>::type>::type>::value) ||

				// We have an array of an allowed char type
				(std::is_array<typename std::remove_reference<StringType>::type>::value && is_char_type<typename std::remove_cv<typename std::remove_pointer<typename std::decay<typename std::remove_reference<StringType>::type>::type>::type>::type>::value) ||

				// We have a non-Plain-Old-Data type. This is MOST LIKELY a basic_string<CharType>. The compiler will complain if it isn't.
				// Other containers from the std don't have the length() method.
				!std::is_pod<typename std::remove_reference<StringType>::type>::value

			>::type

			irender_parameter(const FormatDesc& FormatDescription, StringType&& String, CharBuffer& Output)
			{
				//FormatDescription;
				StringDispatcher(std::forward<StringType>(String), Output);
			}

		};
	};
};

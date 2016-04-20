// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

#include "IntegerRendererHelpers.h"



namespace ts_printf
{
	namespace _details
	{
		template<class char_type>
		class CFloatRenderer;

		template<class char_type>
		class CIntegerRenderer
		{
			typedef SFormatDesc<char_type> SFormatDesc;
			typedef CCharBuffer<char_type> CCharBuffer;
			friend class CFloatRenderer<char_type>;

			template<class T>
			static void irender_actual(const SFormatDesc &fd, T value, CCharBuffer &buffer, size_t allocate_additional, char_type *&start, char_type *&end)
			{

#define XRENDER_COMMAND(base, mode) \
	case base: \
		end = Renderer<T, _details::mode>::prepare_buffer(buffer, allocate_additional, fd.Flags); \
		start = Renderer<T, _details::mode>::render(value, end, fd.Flags); \
		break;


				switch(fd.Base)
				{
					XRENDER_COMMAND(2,  Binary)
					XRENDER_COMMAND(8,  Octal)
					XRENDER_COMMAND(10, Decimal)
					XRENDER_COMMAND(16, Hex)

				default:
					UNREACHABLE;
					break;
				}


#undef XRENDER_COMMAND

			}



		protected:
			

			// We have an integral value. Render it.
			template<class T>
			static std::enable_if_t<std::is_integral<std::remove_reference_t<T>>::value>
			irender_parameter(const SFormatDesc &fd, const T &value, CCharBuffer &buffer)
			{
				char_type *end, *start;

				// Do we want to render as a char?
				if((fd.Flags & E_FORMATFLAGS::FORCECHAR) && (is_char_type<typename std::remove_reference<T>::type>::value))
				{
					start = buffer.Allocate(1);
					*start = static_cast<char_type>(value);
					end = start + 1;
				}
				else
				{
					irender_actual(fd, value, buffer, 0, start, end);
				}

				buffer.SetLength(end - start);
				buffer.SetOffset(start - buffer.GetData());
			}


			// We have a pointer to a non-char type. Forward to render as integral value (uintptr_t).
			template<class T>
			static std::enable_if_t<
				(std::is_pointer<std::remove_reference_t<T>>::value || std::is_array<std::remove_reference_t<T>>::value) &&
				!is_char_type<remove_all_t<T>>::value>
			irender_parameter(const SFormatDesc &fd, const T &pValue, CCharBuffer &buffer)
			{
				irender_parameter(fd, reinterpret_cast<uintptr_t>(pValue), buffer);
			}

		
			// We have an enum value. Forward to render as integral value.
			template<class T>
			static std::enable_if_t<std::is_enum<bare_type_t<T>>::value>
			irender_parameter(const SFormatDesc &fd, const T &eValue, CCharBuffer &buffer)
			{
				typedef std::underlying_type_t<bare_type_t<T>> underlying_type;

				irender_parameter(
					fd,
					static_cast<underlying_type>(eValue),
					buffer);
			}
		};
	}
}


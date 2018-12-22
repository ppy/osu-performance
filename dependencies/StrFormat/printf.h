// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

#include "FormatDesc.h"
#include "ExtendedFormat.h"
#include "BasicAdaptor.h"
#include "CharBuffer.h"
#include "StringRenderer.h"
#include "IntegerRenderer.h"
#include "FloatRenderer.h"
//#include "timedate_renderer.h"


namespace ts_printf
{
	struct no_value_t {};
	//static const no_value_t no_value;

	namespace _details
	{

		// General case undefined
		template<typename CharType>
		struct Ellipsis;

		template<>
		struct Ellipsis<char>
		{
			static const char value='_';
		};

		template<>
		struct Ellipsis<wchar_t>
		{
			static const wchar_t value = L'…';
		};

		template<>
		struct Ellipsis<char16_t>
		{
			//static const char16_t value = u'…';
		};

		template<>
		struct Ellipsis<char32_t>
		{
			//static const char32_t value = U'…';
		};



		template<typename CharType, template<class, class> class CParser>
		class CFormatter
			:
			protected _details::SBaseFormatter,
			public CParser<CFormatter<CharType, CParser>, CharType>,
			protected _details::CStringRenderer<CharType>,
			protected _details::CIntegerRenderer<CharType>,
			protected _details::CFloatRenderer<CharType>
			//protected _details::timedate_renderer<CharType>
		{
			typedef CFormatter thisClass;

			typedef _details::SFormatDesc<CharType> SFormatDesc;
			typedef _details::CCharBuffer<CharType> CCharBuffer;

			// bring parameter rendering implementation functions here
			using _details::CStringRenderer<CharType>::irender_parameter;
			using _details::CIntegerRenderer<CharType>::irender_parameter;
			using _details::CFloatRenderer<CharType>::irender_parameter;
			//using _details::timedate_renderer<CharType>::irender_parameter;

			//
			const CharType* const m_FormatBegin;		// holds either a reference to (const) outside object, or a copy of a temporary
			const CharType* const m_FormatEnd;
			std::unique_ptr<SFormatDesc[]> m_FormatParts;	// contains parsed format string
			size_t m_NumFormatParts;





			void irender_parameter(const SFormatDesc &,const no_value_t &,CCharBuffer &) const
			{
			}




			template<u32 ParamIndex, class CAdaptor, typename A>
			void render_parameter(const SFormatDesc& fd, CAdaptor &adaptor, CCharBuffer& buffer, A&& Arg) const
			{
				if(ParamIndex == fd.ParamIndex)
				{
					irender_parameter(fd, std::forward<A>(Arg), buffer);
				}

				// We are at the end of the "recursion" through the argument list. Finish up by calling the common rendering method.
				// NOTE: This isn't actual recursion, since the methods called in sequence are of DIFFERENT templates and thus not the same memory location
				//       The compiler most likely unrolls it into a simple sequence of statements.
				common_render_parameter(buffer, fd, adaptor);
			}

			template<u32 ParamIndex, class CAdaptor, typename A, typename... B>
			void render_parameter(const SFormatDesc& fd,CAdaptor &adaptor, CCharBuffer& buffer, A&& Arg, B&&... ArgTail) const
			{
				if(ParamIndex == fd.ParamIndex)
				{
					irender_parameter(fd, std::forward<A>(Arg), buffer);
				}

				render_parameter<ParamIndex+1>(fd, adaptor, buffer, std::forward<B>(ArgTail)...);
			}



			template<class CAdaptor>
			void common_render_parameter(CCharBuffer& buffer, const SFormatDesc& fd, CAdaptor& adaptor) const
			{
				const CharType* result = buffer.GetData() + buffer.GetOffset();
				const size_t length = buffer.GetLength();


				if(fd.MinWidth && length < static_cast<size_t>(fd.MinWidth))
				{
					switch (fd.Alignment)
					{
					case E_ALIGNMENT::LEFT:

						adaptor.Write(result, length);
						adaptor.Write(fd.PaddingChar, fd.MinWidth - length);
						return;


					case E_ALIGNMENT::RIGHT:

						adaptor.Write(fd.PaddingChar, fd.MinWidth - length);
						adaptor.Write(result, length);
						return;


					case E_ALIGNMENT::CENTER:

						{
							size_t diff = fd.MinWidth - length;
							adaptor.Write(fd.PaddingChar, diff / 2);
							adaptor.Write(result, length);
							adaptor.Write(fd.PaddingChar, diff - diff / 2);
							return;
						}
					}
				}
				else if(fd.MaxWidth && length > static_cast<size_t>(fd.MaxWidth))
				{
					if(fd.Flags & E_FORMATFLAGS::ELLIPSIS)
					{
						switch (fd.Alignment)
						{
						case E_ALIGNMENT::LEFT:
						case E_ALIGNMENT::CENTER:

							adaptor.Write(result, fd.MaxWidth - 1);
							adaptor.Write(Ellipsis<CharType>::value, 1);
							return;


						case E_ALIGNMENT::RIGHT:

							adaptor.Write(Ellipsis<CharType>::value, 1);
							adaptor.Write(result + length - fd.MaxWidth + 1, fd.MaxWidth - 1);
							return;
						}
					}
					else
					{
						switch (fd.Alignment)
						{
						case E_ALIGNMENT::LEFT:
						case E_ALIGNMENT::CENTER:

							adaptor.Write(result, fd.MaxWidth);
							return;


						case E_ALIGNMENT::RIGHT:

							adaptor.Write(result + length - fd.MaxWidth, fd.MaxWidth);
							return;
						}
					}
				}

				// If we got here, then we don't have to restrict our output! Just write.
				adaptor.Write(result, length);
			}



		public:

			typedef CharType char_type;


			CFormatter(const CharType* t, const CharType* e) throw()
				:
				m_FormatBegin(t), m_FormatEnd(e)
			{
				thisClass::iSetFormat(m_FormatBegin, m_FormatEnd);
			}

			CFormatter()
			{
			}

			CFormatter(const CFormatter &o)
				:
				m_FormatBegin(o.m_FormatBegin),
				m_FormatEnd(o.m_FormatEnd),
				m_NumFormatParts(o.m_NumFormatParts),
				m_FormatParts(new SFormatDesc[o.m_NumFormatParts])
			{
				memcpy(m_FormatParts.get(),o.m_FormatParts.get(),m_NumFormatParts*sizeof(SFormatDesc));
			}

			CFormatter(CFormatter&& other)
				:
				m_FormatBegin(other.m_FormatBegin),
				m_FormatEnd(other.m_FormatEnd),
				m_NumFormatParts(other.m_NumFormatParts),
				m_FormatParts(std::move(other.m_FormatParts))
			{
			}

			CFormatter& operator=(CFormatter other)
			{
				other.Swap(*this);
				return *this;
			}

			void Swap(CFormatter& other)
			{
				std::swap(m_FormatBegin, other.m_FormatBegin);
				std::swap(m_FormatParts, other.m_FormatParts);
				std::swap(m_NumFormatParts, other.m_NumFormatParts);
			}

			// internal, do not use
			void ReserveParts(size_t count) throw()
			{
				m_FormatParts.reset(new SFormatDesc[count]);
				m_NumFormatParts = 0;
			}

			void SetNextPart(SFormatDesc&& part) throw()
			{
				m_FormatParts[m_NumFormatParts++] = std::move(part);
			}

			SFormatDesc& GetNextPart() throw()
			{
				return m_FormatParts[m_NumFormatParts++];
			}

			// rendering to a generalized adaptor
			template<class CAdaptor, typename... A>
			auto render(CAdaptor& Adaptor, A && ... Args) const ->
				std::enable_if_t<std::is_base_of<_details::CBasicAdaptor, CAdaptor>::value>
			{
				CCharBuffer buffer;

				for(u32 i = 0; i < m_NumFormatParts; i++)
				{
					const SFormatDesc& fd = m_FormatParts.get()[i];

					// If we have a normal string, then simply write is
					if(fd.Flags & E_FORMATFLAGS::FORMATSTRING)
					{
						CParser<CFormatter<CharType, CParser>, CharType>::WritePart(Adaptor, fd.Begin, fd.End, fd.bHasEscapedChars);
					}
					// otherwise we need to render the parameter
					else
					{
						buffer.Clear();
						render_parameter<0 /* starts with parameter with index 0 */>(fd, Adaptor, buffer, std::forward<A>(Args)...);
					}
				}
			}
		};

		// Default dispatcher is directly for string objects
		template<template<class, class> class CParser, typename Enable = void>
		struct FormatDispatcher;

		// Default dispatcher is directly for string objects
		template<template<class, class> class CParser, typename CharType>
		struct FormatDispatcher<CParser, std::basic_string<CharType>>
		{
			CFormatter<CharType, CParser> operator ()(std::basic_string<CharType> && FormatString) const
			{
				return CFormatter<CharType, CParser>(FormatString.c_str(), FormatString.c_str() + FormatString.length());
			}
		};

		// Format dispatcher for char pointer
		template<template<class, class> class CParser, typename CharType>
		struct FormatDispatcher<CParser, const CharType *> // DO NOT PUT A CONST AFTER THIS, OR std::basic_string will trigger THIS
		{
			CFormatter<CharType, CParser> operator ()(const CharType* r) const
			{
				return CFormatter<CharType, CParser>(r, r + std::char_traits<CharType>::length(r));
			}
		};

		// Format dispatcher for char array
		template<template<class, class> class CParser, typename CharType, size_t N>
		struct FormatDispatcher<CParser, const CharType(&)[N]>
		{
			CFormatter<CharType, CParser> operator ()(const CharType(&p)[N]) const
			{
				return CFormatter<CharType, CParser>(p, p + std::char_traits<CharType>::length(p));
			}
		};
	}

	// Should be either an accepted char* or std::basic_string<char> whereas char can be any of char, wchat_t, ...
	template<typename StringType>
	auto Format(StringType&& FormatString) ->
		decltype(_details::FormatDispatcher<_details::CExtendedFormat, StringType>()(std::forward<StringType>(FormatString)))
	{
		return _details::FormatDispatcher<_details::CExtendedFormat, StringType>()(std::forward<StringType>(FormatString));
	}
};


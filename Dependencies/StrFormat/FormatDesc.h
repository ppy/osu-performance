// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

namespace ts_printf
{
	namespace _details
	{
		struct SBaseFormatter {};
		
		enum class E_ALIGNMENT : u16
		{
			LEFT,
			CENTER,
			RIGHT,
		};
		
		// Format flags
		enum E_FORMATFLAGS : u32
		{
			NORMAL = 0x00,
			ELLIPSIS = 0x01,	// add … when clipping
			ALWAYSPLUS = 0x02,	// always add + to positive numbers
			LOWHEX = 0x04,	// use lowercase
			FORCECHAR = 0x08,	// display integer as character
			THOUSANDS = 0x10,	// display thousand separators
			TIMEFORMAT = 0x20,	// time format has been specified
			NUMBERPREFIX = 0x40,	// "0x" or "0X" must be rendered before hexadecimal number

			FORMATSTRING = 0x80,	// a reference to an original format string
		};

		template<typename CharType>
		struct SFormatDesc
		{
			// Constructor for text-formats
			SFormatDesc(const CharType* begin, const CharType* end, bool bHasEscapedChars_) throw()
				: Flags(E_FORMATFLAGS::FORMATSTRING), Begin(begin), End(end), bHasEscapedChars(bHasEscapedChars_)
			{
			}

			// Constructor for param-formats
			SFormatDesc() throw()
				:
				Flags(E_FORMATFLAGS::NORMAL),
				ParamIndex(0),
				Alignment(E_ALIGNMENT::LEFT),
				MinWidth(0),
				MaxWidth(0),
				Precision(0),
				PaddingChar(static_cast<CharType>(' ')),
				Base(10)
			{
			}

			E_FORMATFLAGS Flags;				// see FF_* constants. Only BYTE is currently used, but DWORD is chosen for better structure alignment

			union
			{
				struct
				{
					const CharType *tmfBegin;	// the beginning of the time format string
					const CharType *tmfEnd;		// the end of the time format string

					u16 ParamIndex;	// index of the parameter
					E_ALIGNMENT Alignment;
					u16 MinWidth;
					u16 MaxWidth;
					u16 Precision;		// used for floating-point only
					CharType PaddingChar;			// character used for padding
					byte Base;				// used for integer only
				};
				struct
				{
					const CharType* Begin;
					const CharType* End;
					bool bHasEscapedChars;
				};
			};
		};
	};
};


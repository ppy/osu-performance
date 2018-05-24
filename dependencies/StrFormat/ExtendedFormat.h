// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

//////////////////////////////////////////
/*
Format declaration:

{<param-index>[width-decl][alignment-decl][plus-decl][precision-decl][base-decl][padding-decl][ellipsis-decl][char-decl][locale-decl][time-decl]}

To escape the '{' character, duplicate it

<param-index> is zero-based parameter's index and is mandatory
All other parts are optional and may come in any order

[char-decl]
c
Treat the integer parameter as a character (ANSI or UNICODE)

[locale-decl]
l
Separate thousands with default locale's thousand separator

[width-decl]
w<min-width>,<max-width> or
w<min-width> or
w,<max-width>

[alignment-decl]
al | ar | ac
Sets left, right or center alignment

Default alignment is left

[plus-decl]
+
Forces the plus sign for numbers

[precision-decl]
p<number>
Sets the number of digits after the comma

[base-decl]
b<number> or
b[0]16[x] | b[0]16[X]
number may only be 2, 8, 10 or 16. Default is 10. If 'x' is put after 16, lowercase hex is used, otherwise - uppercase (default)
if 0 is specified, "0x" or "0X" will be rendered before the number

[padding-decl]
f<character>
Specify the character to fill when min width is specified. Default is ' '

[ellipsis-decl]
e
Add the ellipsis sign when truncating output. Is not compatible with center alignment (will act as left alignment)

[time-decl]
t(format-string)
Interpret the passed parameter as a date, time or date+time and display, according to format-string (see strftime CRT function)
*/
//////////////////////////////////////////

#include "generic_exception.h"

#include <cassert>

namespace ts_printf
{
	// This exception is thrown if format string cannot be parsed
	struct bad_extended_format_string : generic_exception
	{
	};

	namespace _details
	{
		template<class CFormatter, typename CharType>
		class CExtendedFormat
		{
			typedef SFormatDesc<CharType> FormatDesc;

			template<class IteratorT>
			static u32 _atoi(IteratorT &cur) throw()
			{
				u32 result = 0;
				for (;'0' <= *cur && *cur <= '9'; ++cur)
				{
					result = result * 10 + *cur - '0';
				}
				return result;
			}

		public:

			void iSetFormat(const CharType* cur, const CharType* end)
			{
				CFormatter& This = static_cast<CFormatter&>(*this);


				size_t maxcount = 0;
				for(const char* pos = cur; pos != end; pos++)
				{
					if(*pos == '{')
					{
						maxcount++;
					}
				}


				// Here the maximum possible could of parts is reserved. It may be higher than the actual amount of parts contained in the string,
				// but that would be inefficient to compute, compared to simply reserving a few more pointers.
				This.ReserveParts(maxcount * 2 + 1);


				// Last represents the first character of the last processed part. We start there, at the beginning of the string
				const CharType* last = cur;

				bool bClearEscapes = false;

				// Check every character
				for (; cur != end; ++cur)
				{
					// If it is a '{'...
					if(*cur=='{')
					{
						// ... and afterwards is not the end AND no other '{' (that would escape the whole thing)
						if(cur+1 == end || cur[1] != '{')
						{
							// Then we have a format description that we need to parse.

							// Some text was between this and the last format description. Set the next part as this text
							if(last != cur)
							{
								This.SetNextPart(FormatDesc(last, cur, bClearEscapes));
							}

							bClearEscapes = false;
							++cur;

							// Get a reference to the next parts format description
							FormatDesc& fd = This.GetNextPart();

							// The first char in the format description is the parameter index
							fd.ParamIndex = static_cast<u16>(_atoi(cur));

							while(cur != end)
							{
								switch (*cur)
								{
								case 'a':	// parse alignment
									switch (*++cur)
									{

									case 'l':

										fd.Alignment = E_ALIGNMENT::LEFT;
										break;


									case 'r':

										fd.Alignment = E_ALIGNMENT::RIGHT;
										break;


									case 'c':

										fd.Alignment = E_ALIGNMENT::CENTER;
										break;

									}

									++cur;
									break;


								case '+':

									fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::ALWAYSPLUS);
									++cur;
									break;


								case 'c':

									fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::FORCECHAR);
									++cur;
									break;


								case 't':	// time format

									if (*++cur!='(')
										throw bad_extended_format_string();

									++cur;
									fd.tmfBegin=std::addressof(*cur);
									for (;;++cur)
									{
										if (*cur==')')
										{
											fd.tmfEnd=std::addressof(*cur);
											break;
										} else if (*cur=='}')
											throw bad_extended_format_string();
									}

									fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::TIMEFORMAT);
									++cur;
									break;


								case 'l':

									fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::THOUSANDS);
									++cur;
									break;


								case 'w':

									if (*++cur==',')
									{
										++cur;
										fd.MaxWidth= static_cast<u16>(_atoi(cur));
									}
									else
									{
										u32 Width = _atoi(cur);
										if (*cur == ',')
										{
											fd.MinWidth = static_cast<u16>(Width);

											++cur;

											fd.MaxWidth = static_cast<u16>(_atoi(cur));

											assert(fd.MaxWidth >= fd.MinWidth); // If MinWidth < MaxWidth then something clearly is wrong
										} else
											fd.MinWidth = static_cast<u16>(Width);
									}
									break;


								case 'p':

									fd.Precision = static_cast<u16>(_atoi(++cur));
									break;


								case 'b':

									++cur;

									if(*cur == '0')
									{
										fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::NUMBERPREFIX);
										++cur;
									}

									fd.Base = static_cast<byte>(_atoi(cur));

									// Check if the chosen base is valid
									assert(fd.Base == 2 || fd.Base == 8 || fd.Base == 10 || fd.Base == 16);

									if(fd.Base == 16)
									{
										if(*cur == 'x')
										{
											fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::LOWHEX);
											++cur;
										}
										else if(*cur == 'X')
										{
											++cur;
										}
									}

									break;


								case 'f':

									fd.PaddingChar = *++cur;
									++cur;
									break;


								case 'e':

									fd.Flags = static_cast<E_FORMATFLAGS>(fd.Flags | E_FORMATFLAGS::ELLIPSIS);
									++cur;
									break;


									// Break out of the switch + while loop using an efficient goto. This is probably the only justified use of goto... BUT IT IS JUSTIFIED!
								case '}':

									// The last processed
									last = cur + 1;
									goto endparse;


								default:

									throw bad_extended_format_string();

								}
							}

endparse:			;

						}
						else
						{
							bClearEscapes=true;
							++cur;
						}
					}
				}

				// Add the part that's left in case it isn't the end already. Then we can omit it
				if(last != end)
				{
					This.SetNextPart(FormatDesc(last, end, bClearEscapes));
				}
			}

			template<class CAdaptor>
			static void WritePart(CAdaptor &Adaptor, const CharType *begin, const CharType *end, bool bHasEscapedCharacters) throw()
			{
				if(!bHasEscapedCharacters)
				{
					Adaptor.Write(begin, end - begin);
				}
				else
				{
					const CharType *last = begin;

					for (; begin != end; ++begin)
					{
						if(*begin == '{')
						{
							Adaptor.Write(last, ++begin - last);
							last = begin + 1;
						}
					}
					if(last != end)
					{
						Adaptor.Write(last, end - last);
					}
				}
			}
		};
	};
};

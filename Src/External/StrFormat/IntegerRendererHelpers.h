// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once




#define TSP_BELT_IF(x) typename std::enable_if<std::x<IntType>::value>::type

namespace ts_printf
{
	namespace _details
	{
		// integer number rendering subsystem.
		// FF_LOWHEX in flags is analyzed for base=16
		enum
		{
			Decimal,
			Binary=1,
			Octal=3,
			Hex=4,
		};

		template<class IntType, s32 Base, class Enabler = void>
		struct Renderer
		{
			// generic case is valid for Binary and Octal
			template<class CharType>
			static CharType *prepare_buffer(CCharBuffer<CharType> &CharBuffer, size_t AdditionalToAllocate, E_FORMATFLAGS)
			{
				CharType* Start = CharBuffer.Allocate(AdditionalToAllocate + (sizeof(IntType) * 8 + Base - 1) / Base + 1);
				return Start + CharBuffer.GetSize() - AdditionalToAllocate;
			}

			template<class CharType>
			static CharType *render(IntType Value, CharType *Out, E_FORMATFLAGS /*Flags*/)
			{
				// If value == 0 simply drop out
				if(Value == 0)
				{
					*--Out = '0';
				}
				else
				{
					for(; Value != 0; Value >>= Base)
					{
						*--Out = (Value & ((1 << Base) - 1)) + '0';
					}
				}

				return Out;
			}
		};


		// Int template wrapper
		template<s32 N>
		struct int_
		{
			enum
			{
				value = N
			};
		};


		template<class CharType>
		struct HexStringProvider;

		template<>
		struct HexStringProvider<char>
		{
			static const char **get()
			{
				static const char *hexdigits[] = {
					"0123456789ABCDEF",
					"0123456789abcdef",
				};

				return hexdigits;
			}
		};

		template<>
		struct HexStringProvider<wchar_t>
		{
			static const wchar_t **get()
			{
				static const wchar_t *hexdigits[] = {
					L"0123456789ABCDEF",
					L"0123456789abcdef",
				};

				return hexdigits;
			}
		};

		template<>
		struct HexStringProvider<char16_t>
		{
			/*static const char16_t **get() // not supported by msvc yet
			{
				static const char16_t *hexdigits[] = {
					u"0123456789ABCDEF",
					u"0123456789abcdef",
				};

				return hexdigits;
			}*/
		};

		template<>
		struct HexStringProvider<char32_t>
		{
			/*static const char32_t **get() // not supported by msvc yet
			{
				static const char32_t *hexdigits[] = {
					U"0123456789ABCDEF",
					U"0123456789abcdef",
				};

				return hexdigits;
			}*/
		};



		template<class IntType>
		struct Renderer<IntType, Hex, TSP_BELT_IF(is_unsigned)>
		{
			

			template<class CharType>
			static CharType* prepare_buffer(CCharBuffer<CharType>& CharBuffer, size_t AdditionalToAllocate, E_FORMATFLAGS Flags)
			{
				size_t ToAllocate = sizeof(IntType) * 2 + 1 + AdditionalToAllocate;
				if(Flags & E_FORMATFLAGS::NUMBERPREFIX)
				{
					ToAllocate += 2;
				}

				CharType* ptr = CharBuffer.Allocate(ToAllocate);
				return ptr+CharBuffer.GetSize()-AdditionalToAllocate;
			}

			template<class CharType>
			static CharType *render(IntType val, CharType* Out, E_FORMATFLAGS Flags)
			{
				if(!val)
				{
					*--Out = '0';
				}
				else 
				{
					for(const int array_index = Flags & E_FORMATFLAGS::LOWHEX ? 1 : 0;
						val;
						val >>= 4)
					{
						*--Out=HexStringProvider<CharType>::get()[array_index][val&0xf];
					}
					if(Flags & E_FORMATFLAGS::NUMBERPREFIX)
					{
						*--Out = (Flags & E_FORMATFLAGS::LOWHEX) ? static_cast<CharType>(L'x') : static_cast<CharType>(L'X');
						*--Out = '0';
					}
				}
				return Out;
			}
		};

		struct decimal_render_base
		{
			template<class char_type>
			static char_type GetThousandsSeperator()
			{
				static char_type ThousandsSeperator = std::use_facet<std::numpunct<char_type>>(std::locale("")).thousands_sep();
				return ThousandsSeperator;
			}

			template<size_t Size> struct DecimalSizeMap; // undefined for general case
			template<size_t Size>
			using DecimalSizeMap_t = typename DecimalSizeMap<Size>::type;

		};



		template<> struct decimal_render_base::DecimalSizeMap<1>
		{
			typedef int_<3 + 1> type;
		};

		template<> struct decimal_render_base::DecimalSizeMap<2>
		{
			typedef int_<5 + 1> type;
		};

		template<> struct decimal_render_base::DecimalSizeMap<4>
		{
			typedef int_<10 + 1> type;
		};

		template<> struct decimal_render_base::DecimalSizeMap<8>
		{
			typedef int_<19 + 1> type;
		};



		template<class IntType>
		struct Renderer<IntType, Decimal, TSP_BELT_IF(is_unsigned)> : decimal_render_base
		{
			/*typedef typename boost::mpl::map<
				mpl::pair<mpl::int_<1>,mpl::int_<3+1>>,
				mpl::pair<mpl::int_<2>,mpl::int_<5+1>>,
				mpl::pair<mpl::int_<4>,mpl::int_<10+1>>,
				mpl::pair<mpl::int_<8>,mpl::int_<19+1>>
				> SizeMap;*/

			

			


			template<class CharType>
			static CharType *prepare_buffer(CCharBuffer<CharType>& CharBuffer, size_t AdditionalToAllocate, E_FORMATFLAGS flags)
			{
				auto *ptr = CharBuffer.Allocate(AdditionalToAllocate + DecimalSizeMap_t<sizeof(IntType)>::value + ((flags & E_FORMATFLAGS::THOUSANDS) ? 10 : 0));
				return ptr+CharBuffer.GetSize()-AdditionalToAllocate;
			}

			template<class CharType>
			static CharType *render(IntType val, CharType *Out, E_FORMATFLAGS Flags)
			{
				if(!val)
				{
					*--Out = '0';
				}
				else
				{
					if(Flags & E_FORMATFLAGS::THOUSANDS)
					{
						const CharType thousands_sep = GetThousandsSeperator<CharType>();

						for (int i = 0; val; val /= 10, i++)
						{
							if(i == 3)
							{
								i = 0;
								*--Out = thousands_sep;
							}

							*--Out = '0' + static_cast<CharType>(val % 10);
						}
					}
					else
					{
						for(; val; val /= 10)
						{
							*--Out = '0' + static_cast<CharType>(val % 10);
						}
					}
				}
				return Out;
			}
		};

		

		// support for signed types
		template<class IntType, int mode>
		struct Renderer<IntType, mode, TSP_BELT_IF(is_signed)>
		{
			template<class CharType>
			static CharType *prepare_buffer(CCharBuffer<CharType> &CharBuffer, size_t AdditionalToAllocate, E_FORMATFLAGS Flags)
			{
				return Renderer<typename std::make_unsigned<IntType>::type, mode>::prepare_buffer(CharBuffer, AdditionalToAllocate, Flags);
			}

			template<class CharType>
			static CharType* render(IntType value, CharType *Out, E_FORMATFLAGS Flags)
			{
				Out = Renderer<typename std::make_unsigned<IntType>::type, mode>::render(static_cast<IntType>(std::abs(value)), Out, Flags);

				if(value < 0)
				{
					*--Out = '-';
				}
				// Only print the '+' if we explicitly said that we want to
				else if(Flags & E_FORMATFLAGS::ALWAYSPLUS)
				{
					*--Out = '+';
				}

				return Out;
			}
		};
	};
};

#undef TSP_BELT_IF

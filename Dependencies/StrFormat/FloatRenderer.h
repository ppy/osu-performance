// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

// Implementation of floating-point renderer
// Limitations:
// "Scientific" format is not currently supported
// If precision is not specified, the default one (BELT_TSP_DEFAULT_FP_PRECISION) is used
// This renderer always rounds the floating-point number to the specified precision.
// That is, if precision is 2 and you are outputting a number 7.848, 7.85 is rendered

#if !defined(BELT_TSP_DEFAULT_FP_PRECISION)
#define BELT_TSP_DEFAULT_FP_PRECISION 6
#endif

namespace ts_printf
{
	namespace _details
	{

		class CFloatRendererBase
		{
		protected:
			template<typename FloatType> struct TypeMap; // undefined for general case
			template<typename FloatType>
			using TypeMap_t = typename TypeMap<FloatType>::type;
		};

		template<> struct CFloatRendererBase::TypeMap<float>
		{
			typedef int type;
		};
		
		template<> struct CFloatRendererBase::TypeMap<double>
		{
			typedef long long type;
		};

		template<> struct CFloatRendererBase::TypeMap<long double>
		{
			typedef long long type;
		};

		template<class CharType>
		class CFloatRenderer : public CFloatRendererBase
		{
			typedef SFormatDesc<CharType> FormatDesc;
			typedef CCharBuffer<CharType> CharBuffer;

			static CharType GetDecimalPoint()
			{
				static CharType DecimalPoint = '.';
				return DecimalPoint;
			}

		protected:
			template<class FloatType>
			static std::enable_if_t<std::is_floating_point<FloatType>::value>
				irender_parameter(const FormatDesc& Format, const FloatType& Value, CharBuffer& CharBuffer)
			{
				CharType* Start;
				CharType* End;

				unsigned short Precision = Format.Precision ? Format.Precision : BELT_TSP_DEFAULT_FP_PRECISION;
				
				static const FloatType RoundingTails[] =
				{
					FloatType(5e-1),
					FloatType(5e-2),
					FloatType(5e-3),
					FloatType(5e-4),
					FloatType(5e-5),
					FloatType(5e-6),
					FloatType(5e-7),
					FloatType(5e-8),
					FloatType(5e-9),
					FloatType(5e-10),
					FloatType(5e-11),
				};

				// Precision may not be bigger than 10. We don't have bigger rounding rails
				assert(Precision <= 10);

				// copysign is supposed to be in std, but isn't!
				FloatType val = Value + static_cast<FloatType>(copysign(RoundingTails[Precision], Value));


				FloatType IntPart;

				// copysign is supposed to be in std, but isn't!
				FloatType Fraction =
					static_cast<FloatType>(
						copysign(
							std::modf(val, &IntPart),
							static_cast<FloatType>(1.)
						)
					);


				// Use integer renderer facility to render the integer part of our floating point number
				CIntegerRenderer<CharType>::irender_actual(
					Format,
					static_cast<TypeMap_t<FloatType>>(IntPart),
					CharBuffer,
					Precision + 1,
					Start,
					End);

				
				
				//if(fd.base == 10)
				{
					// Render everything after the decimal point
					*End++ = GetDecimalPoint();
					for(; Precision--;)
					{
						Fraction = std::modf(Fraction * static_cast<FloatType>(Format.Base), &IntPart);
						*End++ = '0' + static_cast<CharType>(IntPart);
					}
				}
				/*else
				{
					// How do you do floats in other bases?
					*end++ = static_cast<CharType>('?');
				}*/

				CharBuffer.SetLength(End - Start);
				CharBuffer.SetOffset(Start - CharBuffer.GetData());
			}
		};
	};
};

// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

#ifndef BELT_TSP_MAX_TIMEDATE_STRING_LENGTH
#define BELT_TSP_MAX_TIMEDATE_STRING_LENGTH 40
#endif

#include <time.h>

namespace ts_printf
{
	struct time_t
	{
		::time_t v;

		time_t(::time_t v_) : v(v_)
		{}

		const ::time_t &get() const
		{
			return v;
		}
	};

	namespace _details
	{
		template<class char_type>
		class timedate_renderer
		{
			typedef FormatDesc<char_type> FormatDesc;
			typedef character_buffer<char_type> character_buffer;

			static const std::locale &get_locale()
			{
				static std::locale loc("");
				return loc;
			}

		protected:
			static void irender_parameter(const FormatDesc &fd,const std::tm &value,character_buffer &buffer)
			{
				if (fd.flags&FF_TIMEFORMAT)
				{
					char_type *begin=buffer.allocate(BELT_TSP_MAX_TIMEDATE_STRING_LENGTH);
					buffer.set_result(
						begin,
						std::use_facet<std::time_put<char_type,char_type *>>(get_locale()).put(begin,*(std::ios_base *) nullptr,fd.pad,&value,fd.tmfBegin,fd.tmfEnd)
					);
				}
			}

			static void irender_parameter(const FormatDesc &fd,const ts_printf::time_t &value,character_buffer &buffer)
			{
				std::tm tm;
				if(!localtime_s(&tm, &value.get()))
					irender_parameter(fd,tm,buffer);
			}

#if defined(_WIN32) && defined(_FILETIME_)
			static void irender_parameter(const FormatDesc &fd,const FILETIME &ftime,character_buffer &buffer)	// in UTC!
			{
				const unsigned long long EpochBias=116444736000000000ull;

		        irender_parameter(fd,(ts_printf::time_t)(((const unsigned long long &)(ftime) - EpochBias) / 10000000ull),buffer);
			}
#endif
		};
	};
};

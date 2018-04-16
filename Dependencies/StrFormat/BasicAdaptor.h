// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

namespace ts_printf
{
	namespace _details
	{
		template<typename T>
		struct is_char_type
		{
			enum
			{
				value = std::is_same<T, char>::value ||
				std::is_same<T, wchar_t>::value ||
				std::is_same<T, char16_t>::value ||
				std::is_same<T, char32_t>::value
			};
		};

		// all output adaptors need to derive from this basic_adaptor struct.
		// it is used to properly match candidate function

		class CBasicAdaptor
		{
		};
	}
};

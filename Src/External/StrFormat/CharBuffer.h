// Type-safe C++0x printf library
// Copyright (c) 2011 HHD Software Ltd. http://www.hhdsoftware.com
// Written by Alexander Bessonov

// Distributed under the terms of The Code Project Open License (http://www.codeproject.com)

#pragma once

#include <array>

// _details::character_buffer class implements temporary storage for renderers
// It uses BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK characters on stack, if it is not enough, it allocates memory from heap

#ifndef BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK
#define BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK 128
#endif

namespace ts_printf
{
	namespace _details
	{
		template<class CharType>
		class CCharBuffer
		{
		public:
			CCharBuffer()
				:
				m_Allocated(BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK),
				m_Used(0)
			{
			}

			void SetLength(size_t Length)
			{
				m_Length = Length;
			}

			size_t GetLength()
			{
				return m_Length;
			}

			void SetOffset(size_t Offset)
			{
				m_Offset = Offset;
			}

			size_t GetOffset()
			{
				return m_Offset;
			}

			void Clear()
			{
				m_Length = 0;
				m_Offset = 0; // Reset offset, so anyone can assumed, that the buffer is -truly- cleared
			}


			CharType* GetData()
			{
				return
					m_Allocated > BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK ?
					m_Heap.get() : m_Stack.data();
			}

			CharType* Allocate(size_t Size)
			{
				if(Size > m_Allocated)
				{
					m_Heap.reset(new CharType[Size]);
					m_Allocated = Size;
				}
					
				m_Used = Size;
				return GetData();
			}



			size_t GetSize() const
			{
				return m_Used;
			}


		private:

			std::array<CharType, BELT_TSP_MAX_TEMPORARY_CHARS_ON_STACK> m_Stack;
			std::unique_ptr<CharType[]> m_Heap;


			size_t m_Allocated;
			size_t m_Used;

			size_t m_Length;
			size_t m_Offset;

			
		};
	}
}

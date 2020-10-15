#include "pch.h"
#include "StackAllocator.h"
#include "Utils.h"  /* CalculatePadding */
#include <algorithm>    /* max */
#ifdef _DEBUG
#include <iostream>
#endif

namespace PEPEngine
{
	namespace Allocator
	{
		StackAllocator::StackAllocator(const std::size_t totalSize)
			: Allocator(totalSize)
		{
		}

		void StackAllocator::Init()
		{
			if (m_start_ptr != nullptr)
			{
				free(m_start_ptr);
			}
			m_start_ptr = malloc(totalSize);
			m_offset = 0;
		}

		StackAllocator::~StackAllocator()
		{
			free(m_start_ptr);
			m_start_ptr = nullptr;
		}

		void* StackAllocator::Allocate(const std::size_t size, const std::size_t alignment)
		{
			const std::size_t currentAddress = (std::size_t)m_start_ptr + m_offset;

			std::size_t padding =
				Utils::CalculatePaddingWithHeader(currentAddress, alignment, sizeof(AllocationHeader));

			if (m_offset + padding + size > totalSize)
			{
				return nullptr;
			}
			m_offset += padding;

			const std::size_t nextAddress = currentAddress + padding;
			const std::size_t headerAddress = nextAddress - sizeof(AllocationHeader);
			AllocationHeader allocationHeader{padding};
			AllocationHeader* headerPtr = (AllocationHeader*)headerAddress;
			headerPtr = &allocationHeader;

			m_offset += size;

#ifdef _DEBUG
			std::cout << "A" << "\t@C " << (void*)currentAddress << "\t@R " << (void*)nextAddress << "\tO " << m_offset
				<< "\tP " << padding << std::endl;
#endif
			used = m_offset;
			peak = std::max(peak, used);

			return (void*)nextAddress;
		}

		void StackAllocator::Free(void* ptr)
		{
			// Move offset back to clear address
			const std::size_t currentAddress = (std::size_t)ptr;
			const std::size_t headerAddress = currentAddress - sizeof(AllocationHeader);
			const AllocationHeader* allocationHeader{(AllocationHeader*)headerAddress};

			m_offset = currentAddress - allocationHeader->padding - (std::size_t)m_start_ptr;
			used = m_offset;

#ifdef _DEBUG
			std::cout << "F" << "\t@C " << (void*)currentAddress << "\t@F " << static_cast<void*>(static_cast<char*>(
				m_start_ptr) + m_offset) << "\tO " << m_offset << std::endl;
#endif
		}

		void StackAllocator::Reset()
		{
			m_offset = 0;
			used = 0;
			peak = 0;
		}
	}
}

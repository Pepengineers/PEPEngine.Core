#include "pch.h"
#include "PoolAllocator.h"
#include <assert.h>
#include <stdint.h>
#include <algorithm>    //max
#ifdef _DEBUG
#include <iostream>
#endif

namespace PEPEngine::Allocator
{
		PoolAllocator::PoolAllocator(const std::size_t totalSize, const std::size_t chunkSize)
			: Allocator(totalSize)
		{
			assert(chunkSize >= 8 && "Chunk size must be greater or equal to 8");
			assert(totalSize % chunkSize == 0 && "Total Size must be a multiple of Chunk Size");
			this->m_chunkSize = chunkSize;
		}

		void PoolAllocator::Init()
		{
			m_start_ptr = malloc(totalSize);
			this->Reset();
		}

		PoolAllocator::~PoolAllocator()
		{
			free(m_start_ptr);
		}

		void* PoolAllocator::Allocate(const std::size_t allocationSize, const std::size_t alignment)
		{
			assert(allocationSize == this->m_chunkSize && "Allocation size must be equal to chunk size");

			Node* freePosition = m_freeList.pop();

			assert(freePosition != nullptr && "The pool allocator is full");

			used += m_chunkSize;
			peak = std::max(peak, used);
#ifdef _DEBUG
			std::cout << "A" << "\t@S " << m_start_ptr << "\t@R " << static_cast<void*>(freePosition) << "\tM " << used
				<< std::endl;
#endif

			return static_cast<void*>(freePosition);
		}

		void PoolAllocator::Free(void* ptr)
		{
			used -= m_chunkSize;

			m_freeList.push(static_cast<Node*>(ptr));

#ifdef _DEBUG
			std::cout << "F" << "\t@S " << m_start_ptr << "\t@F " << ptr << "\tM " << used << std::endl;
#endif
		}

		void PoolAllocator::Reset()
		{
			used = 0;
			peak = 0;
			// Create a linked-list with all free positions
			const int nChunks = totalSize / m_chunkSize;
			for (int i = 0; i < nChunks; ++i)
			{
				std::size_t address = (std::size_t)m_start_ptr + i * m_chunkSize;
				m_freeList.push((Node*)address);
			}
		}
}

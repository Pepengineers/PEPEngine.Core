#include "Allocator.h"
#include "StackLinkedList.h"
#include "pch.h"

namespace PEPEngine
{
	namespace Allocator
	{
		class PoolAllocator : public Allocator
		{
		private:
			struct FreeHeader
			{
			};

			using Node = StackLinkedList<FreeHeader>::Node;
			StackLinkedList<FreeHeader> m_freeList;

			void* m_start_ptr = nullptr;
			std::size_t m_chunkSize;
		public:
			PoolAllocator(std::size_t totalSize, std::size_t chunkSize);

			virtual ~PoolAllocator();

			void* Allocate(std::size_t size, std::size_t alignment = 0) override;

			void Free(void* ptr) override;

			void Init() override;

			virtual void Reset();
		private:
			PoolAllocator(PoolAllocator& poolAllocator);
		};
	}
}

#pragma once

#include "pch.h"
#include "Allocator.h"

namespace PEPEngine
{
	namespace Allocator
	{
		class LinearAllocator : public Allocator
		{
		protected:
			void* startPtr = nullptr;
			std::size_t offset;
		public:
			LinearAllocator(std::size_t totalSize);

			virtual ~LinearAllocator();

			void* Allocate(std::size_t size, std::size_t alignment = 0) override;

			void Free(void* ptr) override;

			void Init() override;

			virtual void Reset();
		};
	}
}

#include "GDescriptorHeap.h"


#include "d3dUtil.h"
#include "GDevice.h"

namespace PEPEngine::Graphics
{
		GDescriptorHeap::GDescriptorHeap(const std::shared_ptr<GDevice> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorsCount)
			: heapType(type)
			  , descriptorCount(descriptorsCount), device(device)
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
			heapDesc.Type = heapType;
			heapDesc.NumDescriptors = descriptorCount;
			heapDesc.Flags = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
				                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
				                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			heapDesc.NodeMask = device->GetNodeMask();

			ThrowIfFailed(device->GetDXDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)));

			baseCPUPtr = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
			baseGPUPtr = descriptorHeap->GetGPUDescriptorHandleForHeapStart();

			descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(heapType);
			freeHandlesCount = descriptorCount;

			AddNewBlock(0, freeHandlesCount);
		}

		D3D12_DESCRIPTOR_HEAP_TYPE GDescriptorHeap::GetType() const
		{
			return heapType;
		}

		uint32_t GDescriptorHeap::FreeHandlerCount() const
		{
			return freeHandlesCount;
		}

		bool GDescriptorHeap::HasSpace(uint32_t descriptorCount) const
		{
			return freeListBySize.lower_bound(descriptorCount) != freeListBySize.end();
		}

		void GDescriptorHeap::AddNewBlock(uint32_t offset, uint32_t descriptorCount)
		{
			const auto offsetIt = freeListByOffset.emplace(offset, descriptorCount);
			const auto sizeIt = freeListBySize.emplace(descriptorCount, offsetIt.first);
			offsetIt.first->second.FreeListBySizeIt = sizeIt;
		}

		GDescriptor GDescriptorHeap::Allocate(uint32_t descriptorCount)
		{
			std::lock_guard<std::mutex> lock(allocationMutex);

			if (descriptorCount > freeHandlesCount)
			{
				return GDescriptor();
			}

			const auto freeBlockIt = freeListBySize.lower_bound(descriptorCount);

			if (freeBlockIt == freeListBySize.end())
			{
				return GDescriptor();
			}

			const auto blockSize = freeBlockIt->first;

			const auto offsetIt = freeBlockIt->second;

			const auto offset = offsetIt->first;

			freeListBySize.erase(freeBlockIt);
			freeListByOffset.erase(offsetIt);

			const auto newOffset = offset + descriptorCount;
			const auto newSize = blockSize - descriptorCount;

			if (newSize > 0)
			{
				AddNewBlock(newOffset, newSize);
			}

			freeHandlesCount -= descriptorCount;

			return GDescriptor(
				CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCPUPtr, offset, descriptorHandleIncrementSize),
				CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGPUPtr, offset, descriptorHandleIncrementSize),
				descriptorCount, descriptorHandleIncrementSize, shared_from_this());
		}

		uint32_t GDescriptorHeap::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
		{
			return static_cast<uint32_t>(handle.ptr - baseCPUPtr.ptr) / descriptorHandleIncrementSize;
		}

		void GDescriptorHeap::Free(GDescriptor&& descriptor, uint64_t frameNumber)
		{
			auto offset = ComputeOffset(descriptor.GetCPUHandle());

			std::lock_guard<std::mutex> lock(allocationMutex);

			staleDescriptors.emplace(offset, descriptor.GetDescriptorCount(), frameNumber);
		}

		void GDescriptorHeap::FreeBlock(uint32_t offset, uint32_t descriptorCount)
		{
			const auto nextBlockIt = freeListByOffset.upper_bound(offset);

			auto prevBlockIt = nextBlockIt;

			if (prevBlockIt != freeListByOffset.begin())
			{
				--prevBlockIt;
			}
			else
			{
				prevBlockIt = freeListByOffset.end();
			}
			freeHandlesCount += descriptorCount;

			if (prevBlockIt != freeListByOffset.end() &&
				offset == prevBlockIt->first + prevBlockIt->second.Size)
			{
				offset = prevBlockIt->first;
				descriptorCount += prevBlockIt->second.Size;

				freeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
				freeListByOffset.erase(prevBlockIt);
			}

			if (nextBlockIt != freeListByOffset.end() &&
				offset + descriptorCount == nextBlockIt->first)
			{
				descriptorCount += nextBlockIt->second.Size;

				freeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
				freeListByOffset.erase(nextBlockIt);
			}

			AddNewBlock(offset, descriptorCount);
		}

		void GDescriptorHeap::ReleaseStaleDescriptors(uint64_t frameNumber)
		{
			std::lock_guard<std::mutex> lock(allocationMutex);

			while (!staleDescriptors.empty() && staleDescriptors.front().FrameNumber <= frameNumber)
			{
				auto& staleDescriptor = staleDescriptors.front();

				const auto offset = staleDescriptor.Offset;
				const auto numDescriptors = staleDescriptor.Size;

				FreeBlock(offset, numDescriptors);

				staleDescriptors.pop();
			}
		}

		ID3D12DescriptorHeap* GDescriptorHeap::GetDirectxHeap() const
		{
			return descriptorHeap.Get();
		}

		std::shared_ptr<GDevice> GDescriptorHeap::GetDevice() const
		{
			return device;
		}
}

#pragma once
#include "d3dx12.h"

namespace PEPEngine
{
	namespace Graphics
	{
		class GDescriptorHeap;

		class GDescriptor
		{
		public:
			GDescriptor();

			GDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			        uint32_t count,
			        uint32_t descriptorSize, std::shared_ptr<GDescriptorHeap> heap);

			~GDescriptor();

			GDescriptor(const GDescriptor&) = delete;
			GDescriptor& operator=(const GDescriptor&) = delete;

			GDescriptor(GDescriptor&& allocation) noexcept;
			GDescriptor& operator=(GDescriptor&& other) noexcept;

			bool IsNull() const;

			D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t offset = 0) const;

			D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t offset = 0) const;

			uint32_t GetDescriptorCount() const;

			D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;

			std::shared_ptr<GDescriptorHeap> GetDescriptorHeap() const;

		private:
			void Free();

			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBase;
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBase;
			uint32_t descriptorCount;
			uint32_t size;

			std::shared_ptr<GDescriptorHeap> page;
		};
	}
}

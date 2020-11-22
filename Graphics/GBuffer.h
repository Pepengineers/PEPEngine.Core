#pragma once
#include "GResource.h"

using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		class GCommandList;

		class GBuffer : public GResource
		{
		protected:
			ComPtr<ID3DBlob> bufferCPU;
			UINT count;
			UINT stride;
			DWORD bufferSize = 0;
			D3D12_GPU_VIRTUAL_ADDRESS address{};
		public:

			GBuffer(std::shared_ptr<GCommandList> cmdList, UINT elementSize, UINT elementCount, void* data, const std::wstring& name = L"", const D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

			GBuffer(std::shared_ptr<GDevice> device, UINT elementSize, UINT elementCount, const std::wstring& name = L"",const D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE , const  D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON,
				D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE);

			void LoadData(const void* data, std::shared_ptr<GCommandList> cmdList);
			void ReadData(void* data) const;

			GBuffer(const GBuffer& rhs);

			GBuffer& operator=(const GBuffer& a);

			D3D12_GPU_VIRTUAL_ADDRESS GetElementResourceAddress(UINT index) const;

			void Reset() override;

			UINT GetElementCount() const;

			DWORD GetBufferSize() const;

			UINT GetStride() const;

			ComPtr<ID3DBlob> GetCPUResource() const;
		};
	}
}
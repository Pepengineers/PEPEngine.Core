#include "DirectXBuffers.h"
#include "GResourceStateTracker.h"

namespace PEPEngine::Graphics
{
	using namespace Utils;

	UploadBuffer::UploadBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize,
	                           std::wstring name, D3D12_RESOURCE_FLAGS flag) :
		GBuffer(device,  name, CD3DX12_RESOURCE_DESC::Buffer(elementCount * elementByteSize, flag), elementByteSize, elementCount, nullptr,
		          D3D12_RESOURCE_STATE_GENERIC_READ, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD))
	{
		ThrowIfFailed(dxResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
	}

	UploadBuffer::~UploadBuffer()
	{
		if (dxResource != nullptr)
			dxResource->Unmap(0, nullptr);
		mappedData = nullptr;
	}

	void UploadBuffer::CopyData(int elementIndex, const void* data, size_t size) const
	{
		memcpy(&mappedData[elementIndex * stride], data, size);
	}


}

#include "DirectXBuffers.h"
#include "GResourceStateTracker.h"

namespace PEPEngine::Graphics
{
	using namespace Utils;

	UploadBuffer::UploadBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize,
	                           const std::wstring& name, D3D12_RESOURCE_FLAGS flag, D3D12_RESOURCE_STATES state, D3D12_HEAP_PROPERTIES heapProp) :
		GBuffer(device, elementByteSize, elementCount, name, flag,
			state, heapProp)
	{
		ThrowIfFailed(dxResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
	}

	UploadBuffer::UploadBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize, UINT aligment,
		const std::wstring& name, D3D12_RESOURCE_FLAGS flag, D3D12_RESOURCE_STATES state, D3D12_HEAP_PROPERTIES heapProp) :
		GBuffer(device, elementByteSize, elementCount, aligment, name, flag,
			state, heapProp)
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

	void UploadBuffer::ReadData(int elementIndex, void* data, size_t size) const
	{
		memcpy(data, &mappedData[elementIndex * stride], size);
	}
}

#include "GBuffer.h"
#include "GCommandList.h"
#include "d3dcompiler.h"
#include "d3dUtil.h"

namespace PEPEngine::Graphics
{
	D3D12_GPU_VIRTUAL_ADDRESS GBuffer::GetElementResourceAddress(UINT index) const
	{
		return address + (stride * index);
	}

	void GBuffer::Reset()
	{
		GResource::Reset();
		address = 0;
	}

	GBuffer::GBuffer(std::shared_ptr<GCommandList> cmdList,
		UINT elementSize, UINT elementCount, void* data,  const std::wstring& name, D3D12_RESOURCE_FLAGS flags) :
		GResource(cmdList->GetDevice(), CD3DX12_RESOURCE_DESC::Buffer(elementSize* elementCount, flags), name), count(elementCount), stride(elementSize), bufferSize(stride* count)
	{
		address = dxResource->GetGPUVirtualAddress();
		LoadData(data, cmdList);
	}

	GBuffer::GBuffer(std::shared_ptr<GDevice> device,
		UINT elementSize, UINT elementCount, const std::wstring& name, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState,
		D3D12_HEAP_PROPERTIES heapProp, D3D12_HEAP_FLAGS heapFlags) : GResource(device, CD3DX12_RESOURCE_DESC::Buffer(elementSize * elementCount,flags), name, nullptr, initState, heapProp, heapFlags), count(elementCount), stride(elementSize), bufferSize(stride* count)
	{
		address = dxResource->GetGPUVirtualAddress();
	}

	GBuffer::GBuffer(std::shared_ptr<GDevice> device,
		UINT elementSize, UINT elementCount, UINT aligment, const std::wstring& name, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState,
		D3D12_HEAP_PROPERTIES heapProp, D3D12_HEAP_FLAGS heapFlags) : GResource(device, CD3DX12_RESOURCE_DESC::Buffer((((elementSize* elementCount) + aligment - 1) & ~(aligment - 1)) + sizeof(UINT), flags), name, nullptr, initState, heapProp, heapFlags), count(elementCount), stride(elementSize), bufferSize((((elementSize* elementCount) + aligment - 1) & ~(aligment - 1)) + sizeof(UINT))
	{
		address = dxResource->GetGPUVirtualAddress();
	}
	
	void GBuffer::LoadData(const void* data, std::shared_ptr<GCommandList> cmdList)
	{
		ThrowIfFailed(D3DCreateBlob(bufferSize, bufferCPU.GetAddressOf()));
		CopyMemory(bufferCPU->GetBufferPointer(), data, bufferSize);

		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = data;
		subResourceData.RowPitch = bufferSize;
		subResourceData.SlicePitch = subResourceData.RowPitch;

		cmdList->UpdateSubresource(dxResource, &subResourceData, 1);
		cmdList->FlushResourceBarriers();
	}

	void GBuffer::ReadData(void* data) const
	{
		ThrowIfFailed(dxResource->Map(0, nullptr, reinterpret_cast<void**>(&data)));

		dxResource->Unmap(0, nullptr);
	}

	GBuffer::GBuffer(const GBuffer& rhs) : GResource(rhs)
	{
		this->address = rhs.address;
		this->bufferCPU = rhs.bufferCPU;
		this->bufferSize = rhs.bufferSize;
		this->stride = rhs.stride;
		this->count = rhs.count;
	}

	GBuffer& GBuffer::operator=(const GBuffer& a)
	{
		GResource::operator=(a);
		this->address = a.address;
		this->bufferCPU = a.bufferCPU;
		this->bufferSize = a.bufferSize;
		this->stride = a.stride;
		this->count = a.count;
		return *this;
	}

	UINT GBuffer::GetElementCount() const
	{
		return count;
	}

	DWORD GBuffer::GetBufferSize() const
	{
		return bufferSize;
	}

	UINT GBuffer::GetStride() const
	{
		return stride;
	}

	ComPtr<ID3DBlob> GBuffer::GetCPUResource() const
	{
		return bufferCPU;
	}
}
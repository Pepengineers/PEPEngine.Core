#include "GMeshBuffer.h"

namespace PEPEngine::Graphics
{
	using namespace Utils;

	D3D12_INDEX_BUFFER_VIEW* GMeshBuffer::IndexBufferView()
	{
		return &ibv;
	}

	D3D12_VERTEX_BUFFER_VIEW* GMeshBuffer::VertexBufferView()
	{
		return &vbv;
	}


	GMeshBuffer GMeshBuffer::CreateBuffer(std::shared_ptr<GCommandList> cmdList, void* data, UINT elementSize,
	                                      UINT count,
	                                      const std::wstring& name)
	{
		const auto desc = CD3DX12_RESOURCE_DESC::Buffer(elementSize * count);
		GMeshBuffer buffer(cmdList, name, desc, elementSize, count, data);
		return buffer;
	}

	GMeshBuffer::GMeshBuffer(const GMeshBuffer& rhs): GBuffer(rhs)
	{
		this->ibv = rhs.ibv;
		this->vbv = rhs.vbv;
	}

	GMeshBuffer& GMeshBuffer::operator=(const GMeshBuffer& a)
	{
		GBuffer::operator=(a);
		this->ibv = a.ibv;
		this->vbv = a.vbv;
		return *this;
	}

	GMeshBuffer::GMeshBuffer(std::shared_ptr<GCommandList> cmdList, const std::wstring& name,
	                         const D3D12_RESOURCE_DESC& resourceDesc, UINT elementSize, UINT elementCount,
	                         void* data) : GBuffer(cmdList, name, resourceDesc, elementSize, elementCount, data)
	{
		ibv.BufferLocation = dxResource->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = bufferSize;

		vbv.BufferLocation = dxResource->GetGPUVirtualAddress();
		vbv.StrideInBytes = stride;
		vbv.SizeInBytes = bufferSize;
	}

	
}

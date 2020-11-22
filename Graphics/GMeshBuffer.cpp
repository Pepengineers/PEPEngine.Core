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
		GMeshBuffer buffer(cmdList,elementSize, count, data, name);
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

	GMeshBuffer::GMeshBuffer(std::shared_ptr<GCommandList> cmdList, UINT elementSize, UINT elementCount,
	                         void* data, const std::wstring& name) : GBuffer(cmdList,  elementSize, elementCount, data, name)
	{
		ibv.BufferLocation = dxResource->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = bufferSize;

		vbv.BufferLocation = dxResource->GetGPUVirtualAddress();
		vbv.StrideInBytes = stride;
		vbv.SizeInBytes = bufferSize;
	}

	
}

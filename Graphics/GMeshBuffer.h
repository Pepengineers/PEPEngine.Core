#pragma once

#include "GBuffer.h"

using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		class GCommandList;		

		class GMeshBuffer : public GBuffer
		{
		protected:
			const DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;

			D3D12_INDEX_BUFFER_VIEW ibv = {};
			D3D12_VERTEX_BUFFER_VIEW vbv = {};

		public:

			static GMeshBuffer CreateBuffer(std::shared_ptr<GCommandList> cmdList, void* data, UINT elementSize,
			                                UINT count, const std::wstring& name = L"");

			GMeshBuffer(const GMeshBuffer& rhs);

			GMeshBuffer& operator=(const GMeshBuffer& a);

			D3D12_INDEX_BUFFER_VIEW* IndexBufferView();

			D3D12_VERTEX_BUFFER_VIEW* VertexBufferView();

		private:
			GMeshBuffer(std::shared_ptr<GCommandList> cmdList, const std::wstring& name,
			            const D3D12_RESOURCE_DESC& resourceDesc,
			            UINT elementSize, UINT elementCount,
			            void* data);

		private:
		};
	}
}

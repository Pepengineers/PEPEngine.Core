#pragma once


#include "d3dUtil.h"
#include "GBuffer.h"
using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		class UploadBuffer : public GBuffer
		{
		public:
			UploadBuffer(std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize,
			             std::wstring name = L"", D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

			UploadBuffer(const UploadBuffer& rhs) = delete;
			UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

			~UploadBuffer();

			void CopyData(int elementIndex, const void* data, size_t size) const;
			
		protected:			
			BYTE* mappedData = nullptr;
		};

		template <typename T>
		class ConstantUploadBuffer : public virtual UploadBuffer
		{
		public:
			// Constant buffer elements need to be multiples of 256 bytes.
			// This is because the hardware can only view constant data
			// at m*256 byte offsets and of n*256 byte lengths.
			// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
			//  UINT64 OffsetInBytes; // multiple of 256
			//  UINT  SizeInBytes;  // multiple of 256
			// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
			ConstantUploadBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, std::wstring name = L"") :
				UploadBuffer(
					device, elementCount, Utils::d3dUtil::CalcConstantBufferByteSize(sizeof(T)), name = L"")
			{
			}

			void CopyData(int elementIndex, const T& data)
			{
				UploadBuffer::CopyData(elementIndex, &data, sizeof(T));
			}
		};

		template <typename T>
		class StructuredUploadBuffer : public virtual UploadBuffer
		{
		public:
			StructuredUploadBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, std::wstring name = L"", D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) :
				UploadBuffer(
					device, elementCount, (sizeof(T)), name, flag)
			{
			}

			void CopyData(int elementIndex, const T& data)
			{
				UploadBuffer::CopyData(elementIndex, &data, sizeof(T));
			}
		};
	}
}

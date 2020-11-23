#pragma once


#include "d3dUtil.h"
#include "GBuffer.h"
#include "GCommandList.h"
using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		class UploadBuffer : public GBuffer
		{
		public:
			UploadBuffer(std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize,
			             const std::wstring& name = L"", D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD));

			UploadBuffer(std::shared_ptr<GDevice> device, UINT elementCount, UINT elementByteSize, UINT aligment,
				const std::wstring& name = L"", D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD));

			UploadBuffer(const UploadBuffer& rhs) = delete;
			UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

			~UploadBuffer();

			void CopyData(int elementIndex, const void* data, size_t size) const;
			void ReadData(int elementIndex, void* data, size_t size) const;

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
					device, elementCount, Utils::d3dUtil::CalcConstantBufferByteSize(sizeof(T)), name)
			{
			}

			void CopyData(int elementIndex, const T& data)
			{
				UploadBuffer::CopyData(elementIndex, &data, sizeof(T));
			}
		};

		template <typename T>
		class ReadBackBuffer : public virtual UploadBuffer
		{
		public:
			// Constant buffer elements need to be multiples of 256 bytes.
			// This is because the hardware can only view constant data
			// at m*256 byte offsets and of n*256 byte lengths.
			// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
			//  UINT64 OffsetInBytes; // multiple of 256
			//  UINT  SizeInBytes;  // multiple of 256
			// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
			ReadBackBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, std::wstring name = L"") :
				UploadBuffer(
					device, elementCount, (sizeof(T)), name, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK))
			{
			}

			void ReadData(int elementIndex, T& data)
			{
				UploadBuffer::ReadData(elementIndex, &data, sizeof(T));
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

			void ReadData(int elementIndex, T& data)
			{
				UploadBuffer::ReadData(elementIndex, &data, sizeof(T));
			}
		};

		template <typename T>
		class UnorderedCounteredStructBuffer: public GBuffer
		{
			std::shared_ptr<UploadBuffer> upload;
			
		public:
			UnorderedCounteredStructBuffer(const std::shared_ptr<GDevice> device, UINT elementCount, std::wstring name = L"", D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT) :
				GBuffer(
					device, (sizeof(T)), elementCount, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT, name, flag, D3D12_RESOURCE_STATE_COMMON, CD3DX12_HEAP_PROPERTIES(heapType))
			{
				if(heapType == D3D12_HEAP_TYPE_DEFAULT)
				{
					upload = std::make_shared<UploadBuffer>(device, 1, (sizeof(T)), name + L" Upload");
				}
			}

			UnorderedCounteredStructBuffer(const UnorderedCounteredStructBuffer& rhs) = delete;
			UnorderedCounteredStructBuffer& operator=(const UnorderedCounteredStructBuffer& rhs) = delete;			

			void SetCounterValue(std::shared_ptr<GCommandList> cmdList, UINT value) const
			{
				upload->CopyData(0, &value, sizeof(UINT));			
				cmdList->CopyBufferRegion(dxResource, bufferSize - sizeof(UINT), upload->GetD3D12Resource(), 0, sizeof(UINT));
			}
		};
	}
}

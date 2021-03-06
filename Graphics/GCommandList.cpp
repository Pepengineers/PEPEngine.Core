#include "GCommandList.h"
#include "ComputePSO.h"
#include "GDataUploader.h"
#include "GResource.h"
#include "GResourceStateTracker.h"
#include "GDescriptor.h"
#include "GraphicPSO.h"
#include "GRootSignature.h"
#include "GDevice.h"
#include "GTexture.h"
#include "GDescriptorHeap.h"
#include "GBuffer.h"
#include "GRenderTarger.h"

namespace PEPEngine::Graphics
{
		void GCommandList::TrackResource(ComPtr<ID3D12Object> object)
		{
			trackedObject.push_back(object);
		}

		void GCommandList::TrackResource(const GResource& res)
		{
			TrackResource(res.GetD3D12Resource());
		}

		std::shared_ptr<GDevice> GCommandList::GetDevice() const
		{
			return queue->device;
		}

		GCommandList::GCommandList(const std::shared_ptr<GCommandQueue> queue, D3D12_COMMAND_LIST_TYPE type)
			: type(type), queue(queue)
		{
			ThrowIfFailed(queue->device->GetDXDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&cmdAllocator)));

			ThrowIfFailed(
				queue->device->GetDXDevice()->CreateCommandList(queue->device->GetNodeMask(), type, cmdAllocator.Get(),
					nullptr, IID_PPV_ARGS(&cmdList)));

			uploadBuffer = std::make_unique<GDataUploader>(queue->device);

			tracker = std::make_unique<GResourceStateTracker>();

			for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
			{
				setedDescriptorHeaps[i] = nullptr;
			}
		}

		GCommandList::~GCommandList()
		{
			uploadBuffer->Clear();
			uploadBuffer.reset();
		}

		void GCommandList::BeginQuery(UINT index) const
		{
			cmdList->BeginQuery(queue->timestampQueryHeap.value().Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
		}

		void GCommandList::EndQuery(UINT index) const
		{
			cmdList->EndQuery(queue->timestampQueryHeap.value().Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
		}

		void GCommandList::ResolveQuery(UINT index, UINT quriesCount, UINT64 aligned) const
		{
			cmdList->ResolveQueryData(queue->timestampQueryHeap.value().Get(), D3D12_QUERY_TYPE_TIMESTAMP, index,
			                          quriesCount,
			                          queue->timestampResultBuffer.value().GetD3D12Resource().Get(), aligned);
		}

		
	
		D3D12_COMMAND_LIST_TYPE GCommandList::GetCommandListType() const
		{
			return type;
		}

		ComPtr<ID3D12GraphicsCommandList2> GCommandList::GetGraphicsCommandList() const
		{
			return cmdList;
		}

		void GCommandList::SetDescriptorsHeap(const GDescriptor* memory)
		{
			if (memory == nullptr || memory->IsNull())
				assert(memory == nullptr || memory->IsNull() && "Memory Descriptor Heap is null");

			const auto type = memory->GetType();
			const auto heap = memory->GetDescriptorHeap()->GetDirectxHeap();

			if (setedDescriptorHeaps[type] != heap)
			{
				setedDescriptorHeaps[type] = heap;

				UpdateDescriptorHeaps();
			}
		}

		void GCommandList::UpdateDescriptorHeaps()
		{
			UINT numDescriptorHeaps = 0;
			ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

			for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
			{
				ID3D12DescriptorHeap* descriptorHeap = setedDescriptorHeaps[i];
				if (descriptorHeap)
				{
					descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
				}
			}

			cmdList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
		}

		void GCommandList::SetRootSignature(GRootSignature* signature)
		{
			const auto d3d12RootSignature = signature->GetRootSignature();

			if (cachedRootSignature == d3d12RootSignature) return;

			cachedRootSignature = d3d12RootSignature;

			if(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRootSignature(cachedRootSignature.Get());				
			}
			else
			{
				cmdList->SetGraphicsRootSignature(cachedRootSignature.Get());
			}
			
			TrackResource(cachedRootSignature);
		}

		void GCommandList::SetRootShaderResourceView(UINT rootSignatureSlot, GBuffer& resource, UINT offset)
		{
			assert(cachedRootSignature != nullptr && "Root Signature not set into the CommandList");

			assert(resource.IsValid() && "Resource is invalid");

			const auto res = resource.GetElementResourceAddress(offset);

			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRootShaderResourceView(rootSignatureSlot, res);
			}
			else
			{
				cmdList->SetGraphicsRootShaderResourceView(rootSignatureSlot, res);
			}			
			
			TrackResource(resource);
		}


		void GCommandList::SetRootConstantBufferView(UINT rootSignatureSlot, GBuffer& resource, UINT offset)
		{
			if (cachedRootSignature == nullptr)
				assert("Root Signature not set into the CommandList");

			if (!resource.IsValid())
				assert("Resource is invalid");

			const auto res = resource.GetElementResourceAddress(offset);

			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRootConstantBufferView(rootSignatureSlot, res);
			}
			else
			{
				cmdList->SetGraphicsRootConstantBufferView(rootSignatureSlot, res);
			}

			TrackResource(resource);
		}


		void GCommandList::SetRootUnorderedAccessView(UINT rootSignatureSlot, GBuffer& resource, UINT offset)
		{
			if (cachedRootSignature == nullptr)
				assert("Root Signature not set into the CommandList");

			if (!resource.IsValid())
				assert("Resource is invalid");

			const auto res = resource.GetElementResourceAddress(offset);

			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRootUnorderedAccessView(rootSignatureSlot, res);
			}
			else
			{
				cmdList->SetGraphicsRootUnorderedAccessView(rootSignatureSlot, res);
			}			

			TrackResource(resource);
		}


		void GCommandList::SetRoot32BitConstants(UINT rootSignatureSlot, UINT Count32BitValueToSet, const void* data,
		                                         UINT DestOffsetIn32BitValueToSet) const
		{
			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRoot32BitConstants(rootSignatureSlot, Count32BitValueToSet, data,
					DestOffsetIn32BitValueToSet);
			}
			else
			{
				cmdList->SetGraphicsRoot32BitConstants(rootSignatureSlot, Count32BitValueToSet, data,
					DestOffsetIn32BitValueToSet);
			}			
		}

		void GCommandList::SetRoot32BitConstant(UINT shaderRegister, UINT value, UINT offset) const
		{
			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRoot32BitConstant(shaderRegister, value, offset);
			}			
			else
			{
				cmdList->SetGraphicsRoot32BitConstant(shaderRegister, value, offset);
			}

		
		}


		void GCommandList::SetRootDescriptorTable(UINT rootSignatureSlot, const GDescriptor* memory, UINT offset) const
		{

			
			if (memory == nullptr || memory->IsNull())
			{
				assert("Memory null");
			}		

			if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
			{
				cmdList->SetComputeRootDescriptorTable(rootSignatureSlot, memory->GetGPUHandle(offset));
			}
			else
			{
				cmdList->SetGraphicsRootDescriptorTable(rootSignatureSlot, memory->GetGPUHandle(offset));
			}
		}

		UploadAllocation GCommandList::UploadData(size_t sizeInBytes, const void* bufferData,
		                                          size_t alignment = 0) const
		{
			const auto allocation = uploadBuffer->Allocate(sizeInBytes, alignment);
			if (bufferData != nullptr)
				memcpy(allocation.CPU, bufferData, sizeInBytes);
			return allocation;
		}

		void GCommandList::UpdateSubresource(GResource& destResource, D3D12_SUBRESOURCE_DATA* subresources,
		                                     size_t countSubresources)
		{
			UpdateSubresource(destResource.GetD3D12Resource(), subresources, countSubresources);			
		}

		void GCommandList::UpdateSubresource(ComPtr<ID3D12Resource> destResource, D3D12_SUBRESOURCE_DATA* subresources,
			size_t countSubresources)
		{
			const auto res = destResource;

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(res.Get(),
				0, countSubresources);

			auto upload = UploadData(uploadBufferSize, nullptr, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

			tracker->TransitionResource(res.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();

			UpdateSubresources(cmdList.Get(),
				res.Get(), &upload.d3d12Resource,
				upload.Offset, 0, countSubresources,
				subresources);

			TrackResource(res.Get());
		}

	
		void GCommandList::SetViewports(const D3D12_VIEWPORT* viewports, size_t count) const
		{
			assert(count < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
			cmdList->RSSetViewports(count, viewports);
		}

		void GCommandList::SetScissorRects(const D3D12_RECT* scissorRects, size_t count) const
		{
			assert(count < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
			cmdList->RSSetScissorRects(count, scissorRects);
		}

		void GCommandList::SetPipelineState(GraphicPSO& pso)
		{
			const auto pipeState = pso.GetPSO();

			if (pipeState == setedPSO) return;

			setedPSO = pipeState;

			cmdList->SetPipelineState(setedPSO.Get());

			TrackResource(pipeState);
		}

		void GCommandList::SetPipelineState(ComputePSO& pso)
		{
			const auto pipeState = pso.GetPSO();

			if (pipeState == setedPSO) return;

			setedPSO = pipeState;

			cmdList->SetPipelineState(setedPSO.Get());

			TrackResource(pipeState);
		}

		void GCommandList::SetVBuffer(UINT slot, UINT count, D3D12_VERTEX_BUFFER_VIEW* views) const
		{
			cmdList->IASetVertexBuffers(slot, count, views);
		}

		void GCommandList::SetIBuffer(D3D12_INDEX_BUFFER_VIEW* views) const
		{
			cmdList->IASetIndexBuffer(views);
		}


		void GCommandList::TransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
		                                     UINT subresource, bool flushBarriers) const
		{
			if (resource)
			{
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON,
				                                                          stateAfter, subresource);
				tracker->ResourceBarrier(barrier);
			}

			if (flushBarriers)
			{
				FlushResourceBarriers();
			}
		}

		void GCommandList::TransitionBarrier(GRenderTexture* render, D3D12_RESOURCE_STATES stateAfter, UINT subresource,
			bool flushBarriers) const
		{
			assert(render != nullptr);
			TransitionBarrier(render->GetRenderTexture()->GetD3D12Resource(), stateAfter, subresource, flushBarriers);
		}

		void GCommandList::UAVBarrier(const GResource& resource, bool flushBarriers) const
		{
			UAVBarrier(resource.GetD3D12Resource(), flushBarriers);
		}

		void GCommandList::UAVBarrier(ComPtr<ID3D12Resource> resource, bool flushBarriers) const
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());

			tracker->ResourceBarrier(barrier);

			if (flushBarriers)
			{
				FlushResourceBarriers();
			}
		}

		void GCommandList::AliasingBarrier(const GResource& beforeResource, const GResource& afterResource,
		                                   bool flushBarriers) const
		{
			AliasingBarrier(beforeResource.GetD3D12Resource(), afterResource.GetD3D12Resource(), flushBarriers);
		}

		void GCommandList::AliasingBarrier(ComPtr<ID3D12Resource> beforeResource,
		                                   ComPtr<ID3D12Resource> afterResource, bool flushBarriers) const
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource.Get(), afterResource.Get());

			tracker->ResourceBarrier(barrier);

			if (flushBarriers)
			{
				FlushResourceBarriers();
			}
		}

		void GCommandList::FlushResourceBarriers() const
		{
			tracker->FlushResourceBarriers(cmdList);
		}

		void GCommandList::TransitionBarrier(const GResource& resource, D3D12_RESOURCE_STATES stateAfter,
		                                     UINT subresource,
		                                     bool flushBarriers) const
		{
			TransitionBarrier(resource.GetD3D12Resource(), stateAfter, subresource, flushBarriers);
		}

		void GCommandList::CopyTextureRegion(ComPtr<ID3D12Resource> dstRes, UINT DstX,
		                                     UINT DstY,
		                                     UINT DstZ, ComPtr<ID3D12Resource> srcRes, const D3D12_BOX* srcBox)
		{
			TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
			TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);
			FlushResourceBarriers();

			cmdList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(dstRes.Get()), DstX, DstY, DstZ,
			                           &CD3DX12_TEXTURE_COPY_LOCATION(srcRes.Get()), srcBox);

			TrackResource(dstRes.Get());
			TrackResource(srcRes.Get());
		}

		void GCommandList::CopyCounter(ComPtr<ID3D12Resource> dest_resource, UINT dest_offset, ComPtr<ID3D12Resource> source_resource)
		{
			TransitionBarrier(dest_resource, D3D12_RESOURCE_STATE_COPY_DEST);
			TransitionBarrier(source_resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
			FlushResourceBarriers();
			cmdList->CopyBufferRegion(dest_resource.Get(), dest_offset, source_resource.Get(), 0, 4);
		}

		void GCommandList::CopyTextureRegion(const GResource& dstRes, UINT DstX,
		                                     UINT DstY,
		                                     UINT DstZ, const GResource& srcRes, const D3D12_BOX* srcBox)
		{
			CopyTextureRegion(dstRes.GetD3D12Resource(), DstX, DstY, DstZ, srcRes.GetD3D12Resource(), srcBox);
		}

		void GCommandList::CopyBufferRegion(const GBuffer& dstRes, UINT DstOffset,
			const GBuffer& srcRes, UINT SrcOffset, UINT numBytes, bool copyBarier)
		{
			CopyBufferRegion(dstRes.GetD3D12Resource(), DstOffset, srcRes.GetD3D12Resource(), SrcOffset, numBytes, copyBarier);
		}

		void GCommandList::CopyBufferRegion(ComPtr<ID3D12Resource> dstRes, UINT DstOffset,
			const ComPtr<ID3D12Resource> srcRes, UINT SrcOffset, UINT numBytes, bool copyBarier)
		{
			if (copyBarier)
			{
				TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
				TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);
				FlushResourceBarriers();
			}
			cmdList->CopyBufferRegion(dstRes.Get(), DstOffset, srcRes.Get(), SrcOffset, numBytes);
			TrackResource(dstRes.Get());
			TrackResource(srcRes.Get());
		}
	
		void GCommandList::CopyResource(ComPtr<ID3D12Resource> dstRes, ComPtr<ID3D12Resource> srcRes)
		{
			TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
			TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

			FlushResourceBarriers();

			cmdList->CopyResource(dstRes.Get(), srcRes.Get());

			TrackResource(dstRes.Get());
			TrackResource(srcRes.Get());
		}

		void GCommandList::CopyResource(const GResource& dstRes, const GResource& srcRes)
		{
			CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());
		}

		void GCommandList::ResolveSubresource(GResource& dstRes, const GResource& srcRes, uint32_t dstSubresource,
		                                      uint32_t srcSubresource)
		{
			TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
			TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

			FlushResourceBarriers();

			cmdList->ResolveSubresource(dstRes.GetD3D12Resource().Get(), dstSubresource,
			                            srcRes.GetD3D12Resource().Get(),
			                            srcSubresource, dstRes.GetD3D12ResourceDesc().Format);

			TrackResource(srcRes);
			TrackResource(dstRes);
		}

		void GCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex,
		                        uint32_t startInstance) const
		{
			FlushResourceBarriers();

			cmdList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
		}

		void GCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex,
		                               int32_t baseVertex,
		                               uint32_t startInstance) const
		{
			FlushResourceBarriers();

			cmdList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
		}

		void GCommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const
		{
			FlushResourceBarriers();
			cmdList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
		}

		void GCommandList::ReleaseTrackedObjects()
		{
			trackedObject.clear();
		}

		void GCommandList::Reset(GraphicPSO* pso)
		{
			ThrowIfFailed(cmdAllocator->Reset());

			tracker->Reset();
			uploadBuffer->Reset();

			ReleaseTrackedObjects();

			cachedRootSignature = nullptr;
			setedPSO = nullptr;
			setedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

			for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
			{
				setedDescriptorHeaps[i] = nullptr;
			}

			if (pso != nullptr)
			{
				setedPSO = pso->GetPSO();
				TrackResource(setedPSO);
			}

			ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), setedPSO.Get()));
		}

		bool GCommandList::Close(std::shared_ptr<GCommandList>& pendingCommandList) const
		{
			// Flush any remaining barriers.
			FlushResourceBarriers();

			cmdList->Close();

			uint32_t numPendingBarriers = 0;

			if (pendingCommandList != nullptr)
			{
				const auto peddingCmdList = pendingCommandList->GetGraphicsCommandList();

				// Flush pending resource barriers.
				numPendingBarriers = tracker->FlushPendingResourceBarriers(peddingCmdList);
			}
			// Commit the final resource state to the global state.
			tracker->CommitFinalResourceStates();

			return numPendingBarriers > 0;
		}

		void GCommandList::Close() const
		{
			FlushResourceBarriers();
			cmdList->Close();
		}

		void GCommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
		{
			if (setedPrimitiveTopology == primitiveTopology) return;

			setedPrimitiveTopology = primitiveTopology;
			cmdList->IASetPrimitiveTopology(setedPrimitiveTopology);
		}

		void GCommandList::ClearRenderTarget(GDescriptor* memory, size_t offset, const FLOAT rgba[4], D3D12_RECT* rects,
		                                     size_t rectCount) const
		{
			if (memory == nullptr || memory->IsNull())
			{
				assert("Bad Clear Render Target");
			}
			cmdList->ClearRenderTargetView(memory->GetCPUHandle(offset), rgba, rectCount, rects);
		}

		void GCommandList::ClearRenderTarget(GRenderTexture& target, const FLOAT rgba[4], D3D12_RECT* rects,
			size_t rectCount) const
		{
			cmdList->ClearRenderTargetView(target.GetRTVCpu(), rgba, rectCount, rects);
		}

		void GCommandList::SetRenderTargets(size_t RTCount, GDescriptor* rtvMemory, size_t rtvOffset, GDescriptor* dsvMemory,
		                                    size_t dsvOffset) const
		{
			auto* rtvPtr = (rtvMemory == nullptr || rtvMemory->IsNull())
				               ? nullptr
				               : &rtvMemory->GetCPUHandle(rtvOffset);

			auto* dsvPtr = (dsvMemory == nullptr || dsvMemory->IsNull())
				               ? nullptr
				               : &dsvMemory->GetCPUHandle(dsvOffset);

			cmdList->OMSetRenderTargets(RTCount, rtvPtr, true, dsvPtr);
		}

		void GCommandList::SetRenderTarget(GRenderTexture& target, GDescriptor* dsvMemory, size_t dsvOffset) const
		{
			auto* dsvPtr = (dsvMemory == nullptr || dsvMemory->IsNull())
				? nullptr
				: &dsvMemory->GetCPUHandle(dsvOffset);
			
			cmdList->OMSetRenderTargets(1, &target.GetRTVCpu(), true, dsvPtr);
		}

		void GCommandList::SetRenderTargets(GRenderTexture* targets, UINT targetsSize, GDescriptor* dsvMemory,
			size_t dsvOffset, BOOL isSingleHandle) const
		{

			assert(targetsSize != 0);
			
			auto* dsvPtr = (dsvMemory == nullptr || dsvMemory->IsNull())
				? nullptr
				: &dsvMemory->GetCPUHandle(dsvOffset);
			
			if(isSingleHandle)
			{
				cmdList->OMSetRenderTargets(targetsSize, &targets[0].GetRTVCpu(), true, dsvPtr);
			}
			else
			{
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handlers;
				handlers.resize(targetsSize);

				for (int i = 0; i < targetsSize; ++i)
				{
					handlers.push_back(targets[i].GetRTVCpu());
				}

				cmdList->OMSetRenderTargets(handlers.size(), handlers.data(), false, dsvPtr);
				
			}
		}

		void GCommandList::ClearDepthStencil(GDescriptor* dsvMemory, size_t dsvOffset, D3D12_CLEAR_FLAGS flags,
		                                     FLOAT depthValue,
		                                     UINT stencilValue, D3D12_RECT* rects, size_t rectCount) const
		{
			cmdList->ClearDepthStencilView(dsvMemory->GetCPUHandle(dsvOffset), flags, depthValue, stencilValue,
			                               rectCount,
			                               rects);
		}

		
}

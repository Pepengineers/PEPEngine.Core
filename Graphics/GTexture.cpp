#include "GTexture.h"
#include "DirectXTex.h"
#include "filesystem"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "ResourceUploadBatch.h"
#include <winrt/base.h>
#include "GDataUploader.h"
#include "GDescriptor.h"
#include "GResourceStateTracker.h"
#include "GCommandList.h"
#include "GDevice.h"
#include "GShader.h"
#include "GDeviceFactory.h"

namespace PEPEngine::Graphics
{
		std::wstring GTexture::GetFilePath() const
		{
			return filePath;
		}

		void GTexture::Resize(GTexture& texture, uint32_t width, uint32_t height, uint32_t depthOrArraySize)
		{
			if (texture.dxResource)
			{
				GResourceStateTracker::RemoveGlobalResourceState(texture.dxResource.Get());

				CD3DX12_RESOURCE_DESC resDesc(texture.dxResource->GetDesc());
				resDesc.Width = std::max(width, 1u);
				resDesc.Height = std::max(height, 1u);
				resDesc.DepthOrArraySize = depthOrArraySize;

				auto device = texture.device->GetDXDevice();
								
				ThrowIfFailed(device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&resDesc,
					D3D12_RESOURCE_STATE_COMMON,
					texture.clearValue.get(),
					IID_PPV_ARGS(texture.dxResource.ReleaseAndGetAddressOf())
				));

				texture.dxResource->SetName(texture.GetName().c_str());

				GResourceStateTracker::AddGlobalResourceState(texture.dxResource.Get(), D3D12_RESOURCE_STATE_COMMON);
			}
		}

		void GTexture::GenerateMipMaps(std::shared_ptr<GCommandList> cmdList, GTexture** textures, size_t count)
		{
			UINT requiredHeapSize = 0;

			for (int i = 0; i < count; ++i)
			{
				requiredHeapSize += textures[i]->GetD3D12Resource()->GetDesc().MipLevels - 1;
			}

			if (requiredHeapSize == 0)
			{
				return;
			}


			auto mipMapsMemory = cmdList->GetDevice()->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			                                                               2 * requiredHeapSize);


			CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
			srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
			srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

			D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplerDesc.MinLOD = 0.0f;
			samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerDesc.MaxAnisotropy = 0;
			samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			samplerDesc.ShaderRegister = 0;
			samplerDesc.RegisterSpace = 0;
			samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			GRootSignature signature;
			signature.AddConstantParameter(2, 0);
			signature.AddDescriptorParameter(&srvCbvRanges[0], 1);
			signature.AddDescriptorParameter(&srvCbvRanges[1], 1);
			signature.AddStaticSampler(samplerDesc);
			signature.Initialize(textures[0]->device);

			auto shader = std::make_unique<GShader>(L"Shaders\\MipMapCS.hlsl", ComputeShader, nullptr,
			                                        "GenerateMipMaps",
			                                        "cs_5_1");
			shader->LoadAndCompile();


			ComputePSO genMipMapPSO;
			genMipMapPSO.SetShader(shader.get());
			genMipMapPSO.SetRootSignature(signature);
			genMipMapPSO.Initialize(textures[0]->device);

			cmdList->SetRootSignature(&signature);
			cmdList->SetPipelineState(genMipMapPSO);

			cmdList->SetDescriptorsHeap(&mipMapsMemory);

			D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
			srcTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srcTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

			D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
			destTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;


			size_t cpuOffset = 0;
			size_t gpuOffset = 0;

			for (int i = 0; i < count; ++i)
			{
				auto tex = textures[i];

				auto texture = tex->GetD3D12Resource().Get();
				auto textureDesc = texture->GetDesc();

				for (uint32_t TopMip = 0; TopMip < textureDesc.MipLevels - 1; TopMip++)
				{
					uint32_t dstWidth = std::max(static_cast<uint32_t>(textureDesc.Width >> (TopMip + 1)),
					                             static_cast<uint32_t>(1));
					uint32_t dstHeight = std::max(static_cast<uint32_t>(textureDesc.Height >> (TopMip + 1)),
					                              static_cast<uint32_t>(1));

					srcTextureSRVDesc.Format = GetUAVCompatableFormat(textureDesc.Format);
					srcTextureSRVDesc.Texture2D.MipLevels = 1;
					srcTextureSRVDesc.Texture2D.MostDetailedMip = TopMip;
					textures[i]->CreateShaderResourceView(&srcTextureSRVDesc, &mipMapsMemory, (cpuOffset));
					cpuOffset++;

					destTextureUAVDesc.Format = GetUAVCompatableFormat(textureDesc.Format);
					destTextureUAVDesc.Texture2D.MipSlice = TopMip + 1;
					textures[i]->CreateUnorderedAccessView(&destTextureUAVDesc, &mipMapsMemory, (cpuOffset));
					cpuOffset++;


					DirectX::SimpleMath::Vector2 texelSize = DirectX::SimpleMath::Vector2{
						(1.0f / dstWidth), (1.0f / dstHeight)
					};
					cmdList->SetRoot32BitConstants(0, 2, &texelSize, 0);
					cmdList->SetRootDescriptorTable(1, &mipMapsMemory, gpuOffset);
					gpuOffset++;

					cmdList->SetRootDescriptorTable(2, &mipMapsMemory, gpuOffset);
					gpuOffset++;

					cmdList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

					cmdList->UAVBarrier((texture));
				}
			}
		}

		TextureUsage GTexture::GetTextureType() const
		{
			return usage;
		}


		GTexture::GTexture(std::wstring name, TextureUsage use) : GResource(name),
		                                                          usage(use)
		{
		}

		GTexture::GTexture(const std::shared_ptr<GDevice> device, const D3D12_RESOURCE_DESC& resourceDesc,
		                   const std::wstring& name, TextureUsage textureUsage, const D3D12_CLEAR_VALUE* clearValue) :
			GResource(device, resourceDesc, name, clearValue),
			usage(textureUsage)
		{
		}

		GTexture::GTexture(const std::shared_ptr<GDevice> device, ComPtr<ID3D12Resource> resource,
		                   TextureUsage textureUsage,
		                   const std::wstring& name) : GResource(device, resource, name), usage(textureUsage)
		{
		}

		GTexture::GTexture(const GTexture& copy) : GResource(copy)
		{
		}

		GTexture::GTexture(GTexture&& copy) : GResource(copy)
		{
		}

		GTexture& GTexture::operator=(const GTexture& other)
		{
			GResource::operator=(other);

			return *this;
		}

		GTexture& GTexture::operator=(GTexture&& other)
		{
			GResource::operator=(other);

			return *this;
		}


		GTexture::~GTexture()
		{
		}


		DXGI_FORMAT GTexture::GetUAVCompatableFormat(DXGI_FORMAT format)
		{
			DXGI_FORMAT uavFormat = format;

			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
				uavFormat = DXGI_FORMAT_R32_FLOAT;
				break;
			}

			return uavFormat;
		}


		std::shared_ptr<GTexture> GTexture::LoadTextureFromFile(std::wstring filepath,
		                                                        std::shared_ptr<GCommandList> commandList,
		                                                        TextureUsage usage)
		{
			HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);		

			std::filesystem::path filePath(filepath);
			if (!exists(filePath))
			{
				assert("File not found.");
			}


			DirectX::TexMetadata metadata;
			DirectX::ScratchImage scratchImage;


			UINT resFlags = D3D12_RESOURCE_FLAG_NONE;

			if (filePath.extension() == ".dds" || filePath.extension() == ".DDS")
			{
				ThrowIfFailed(
					DirectX::LoadFromDDSFile(filepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage));
			}
			else if (filePath.extension() == ".hdr" || filePath.extension() == ".HDR")
			{
				ThrowIfFailed(DirectX::LoadFromHDRFile(filepath.c_str(), &metadata, scratchImage));
			}
			else if (filePath.extension() == ".tga" || filePath.extension() == ".TGA")
			{
				ThrowIfFailed(DirectX::LoadFromTGAFile(filepath.c_str(), &metadata, scratchImage));
				resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}
			else
			{
				ThrowIfFailed(
					DirectX::LoadFromWICFile(filepath.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratchImage));

				//���� ��� �� DDS ��� "�����������" ��������, �� ��� ��� ����� ����� ������������ �������
				//�� ����� ���� ����������� ������� �� UAV
				resFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}

			/*���� �������� ����� ��������.*/
			if (usage == TextureUsage::Albedo && filePath.extension() != ".dds")
			{
				metadata.format = GetUAVCompatableFormat(metadata.format);
			}

			D3D12_RESOURCE_DESC desc = {};
			desc.Width = static_cast<UINT>(metadata.width);
			desc.Height = static_cast<UINT>(metadata.height);

			/*
			 * DDS �������� ������ ������������ ��� UAV ��� ��������� ������ ����.
			 */

			desc.MipLevels = resFlags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
				                 ? 0
				                 : static_cast<UINT16>(metadata.mipLevels);
			desc.DepthOrArraySize = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
				                        ? static_cast<UINT16>(metadata.depth)
				                        : static_cast<UINT16>(metadata.arraySize);
			desc.Format = metadata.format;
			desc.Flags = static_cast<D3D12_RESOURCE_FLAGS>(resFlags);
			desc.SampleDesc.Count = 1;
			desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

			auto ownerDevice = commandList->GetDevice();

			auto tex = std::make_shared<GTexture>(ownerDevice, desc, filepath, usage);
			tex->filePath = filePath;


			if (tex->GetD3D12Resource())
			{
				std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
				ThrowIfFailed(
					PrepareUpload(ownerDevice->GetDXDevice().Get(), scratchImage.GetImages(), scratchImage.GetImageCount
						(),
						scratchImage.GetMetadata(),
						subresources));

				commandList->TransitionBarrier(tex->GetD3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST);
				commandList->FlushResourceBarriers();


				commandList->UpdateSubresource(*tex.get(), subresources.data(), subresources.size());

				commandList->TransitionBarrier(tex->GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);
				commandList->FlushResourceBarriers();

				if (resFlags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
				{
					tex->HasMipMap = false;
				}
			}

			return tex;
		}

		void GTexture::ClearTrack()
		{
			track.clear();
		}

		bool GTexture::IsUAVCompatibleFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SINT:
				return true;
			default:
				return false;
			}
		}

		bool GTexture::IsSRGBFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return true;
			default:
				return false;
			}
		}

		bool GTexture::IsBGRFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return true;
			default:
				return false;
			}
		}

		bool GTexture::IsDepthFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_D16_UNORM:
				return true;
			default:
				return false;
			}
		}

		DXGI_FORMAT GTexture::GetTypelessFormat(DXGI_FORMAT format)
		{
			DXGI_FORMAT typelessFormat = format;

			switch (format)
			{
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
				break;
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
				break;
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
				typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
				break;
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
				typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
				break;
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
				typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
				break;
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
				typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
				break;
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
				typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
				break;
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
				typelessFormat = DXGI_FORMAT_R32_TYPELESS;
				break;
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
				typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
				break;
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
				typelessFormat = DXGI_FORMAT_R16_TYPELESS;
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
				typelessFormat = DXGI_FORMAT_R8_TYPELESS;
				break;
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
				break;
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
				break;
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
				break;
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM:
				typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
				break;
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM:
				typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
				break;
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
				break;
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
				break;
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
				typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
				break;
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
				break;
			}

			return typelessFormat;
		}


		std::wstring& GTexture::GetName()
		{
			return resourceName;
		}
}

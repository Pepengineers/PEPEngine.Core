#pragma once


#include <memory>

#include "GShader.h"

using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		class RenderMode
		{
		public:
			enum Mode
			{
				SkyBox,
				Opaque,
				Wireframe,
				OpaqueAlphaDrop,
				AlphaSprites,
				Mirror,
				Reflection,
				ShadowMapOpaque,
				ShadowMapOpaqueDrop,
				Transparent,
				DrawNormalsOpaque,
				DrawNormalsOpaqueDrop,
				Ssao,
				SsaoBlur,
				Debug,
				Quad,
				UI,
				Count
			};
		};

		class GDevice;

		class GraphicPSO
		{
			ComPtr<ID3D12PipelineState> pipelineStateObject;
			RenderMode::Mode type;
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

			bool isInitial = false;

		public:

			GraphicPSO(RenderMode::Mode type = RenderMode::Opaque);

			ComPtr<ID3D12PipelineState> GetPSO() const;

			void SetPsoDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);

			D3D12_GRAPHICS_PIPELINE_STATE_DESC GetPsoDescription() const;

			void SetRootSignature(ID3D12RootSignature* rootSign);

			void SetInputLayout(D3D12_INPUT_LAYOUT_DESC layoutDesc);

			void SetRasterizationState(D3D12_RASTERIZER_DESC rastState);

			void SetRenderTargetBlendState(UINT index, D3D12_RENDER_TARGET_BLEND_DESC desc);

			void SetBlendState(D3D12_BLEND_DESC blendDesc);

			void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC ddsDesc);

			void SetDSVFormat(DXGI_FORMAT format);

			void SetSampleMask(UINT sampleMask);

			void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType);

			void SetRenderTargetsCount(UINT count);

			void SetRTVFormat(UINT index, DXGI_FORMAT format);

			void SetSampleCount(UINT count);

			void SetSampleQuality(UINT quality);

			void SetShader(GShader* shader);

			RenderMode::Mode GetType() const;

			void Initialize(std::shared_ptr<GDevice> device);
		};
	}
}

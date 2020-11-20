#include "ComputePSO.h"
#include "GDevice.h"
#include "GShader.h"

namespace PEPEngine::Graphics
{
		ComputePSO::ComputePSO()
		{
		}

		ComputePSO::~ComputePSO()
		{
			pso.ReleaseAndGetAddressOf();
			computePSOdesc = {};
		}

		void ComputePSO::Initialize(const std::shared_ptr<GDevice> device)
		{
			const auto result = device->GetDXDevice()->CreateComputePipelineState(&computePSOdesc, IID_PPV_ARGS(&pso));


			ThrowIfFailed(result);
		}
	
		ComPtr<ID3D12PipelineState> ComputePSO::GetPSO() const
		{
			return pso;
		}

		void ComputePSO::SetRootSignature(GRootSignature& sign)
		{
			computePSOdesc.pRootSignature = sign.GetRootSignature().Get();
		}

		void ComputePSO::SetShader(GShader* shader)
		{
			if (shader->GetType() == ComputeShader)
			{
				computePSOdesc.CS = shader->GetShaderResource();
				return;
			}

			assert("Bad Shader");
		}
}

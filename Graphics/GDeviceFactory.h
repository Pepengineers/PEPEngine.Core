#pragma once
#include <wrl/client.h>
#include  "dxgi1_6.h"
#include "MemoryAllocator.h"
#include "Lazy.h"

using namespace Microsoft::WRL;

namespace PEPEngine
{
	namespace Graphics
	{
		using namespace Allocator;
		using namespace Utils;

		enum GraphicsAdapter : UINT
		{
			GraphicAdapterPrimary = 0,
			GraphicAdapterSecond = GraphicAdapterPrimary + 1,
			GraphicAdapterCount = GraphicAdapterSecond + 1
		};

		class GDevice;

		class GDeviceFactory
		{
			static ComPtr<IDXGIFactory4> CreateFactory();
			static ComPtr<IDXGIFactory4> dxgiFactory;

			static bool CheckTearingSupport();
			static Lazy<bool> isTearingSupport;

			static custom_vector<Lazy<std::shared_ptr<GDevice>>> CreateDevices();
			static custom_vector<Lazy<std::shared_ptr<GDevice>>> hardwareDevices;
			static std::shared_ptr<GDevice> wrapDevice;

			static custom_vector<ComPtr<IDXGIAdapter3>> CreateAdapters();
			static custom_vector<ComPtr<IDXGIAdapter3>> adapters;

		public:

			static ComPtr<IDXGISwapChain4> CreateSwapChain(std::shared_ptr<GDevice> device, DXGI_SWAP_CHAIN_DESC1& desc,
			                                               HWND hwnd);

			static ComPtr<IDXGIFactory4> GetFactory();

			static std::shared_ptr<GDevice> GetDevice(GraphicsAdapter adapter = GraphicAdapterPrimary);

			static custom_vector<std::shared_ptr<GDevice>> GetAllDevices(bool useWrap = false);

			static bool IsTearingSupport();
		};
	}
}

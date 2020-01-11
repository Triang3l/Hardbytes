#include "HbGPU.h"
#include "HbReport.h"
#if defined(HbPlatform_OS_Microsoft)
#include <Windows.h>
#endif

/*******************
 * System API setup
 *******************/

void HbGPU_Root_Init(HbGPU_Root * const root, HbBool validateUsage, HbMem_Tag_Root * const memTagRoot) {
	HbReport_Assert_Assume(root != NULL);
	HbReport_Assert_Assume(memTagRoot != NULL);
	root->memTagRoot_e = memTagRoot;
	root->validateUsage_r = validateUsage; // Validation layer may fail to load, but still save the original intention.
	root->supportedAPIs_r = 0;

	#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
	{
		#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		HMODULE dxgiModule = LoadLibraryW(L"dxgi.dll");
		if (dxgiModule != NULL) {
			HbBool moduleLoaded = HbTrue;
			moduleLoaded &= (*((FARPROC *) &root->api_i.d3d12_i.createDXGIFactory2_i) = GetProcAddress(dxgiModule, "CreateDXGIFactory2")) != NULL;
			moduleLoaded &= (*((FARPROC *) &root->api_i.d3d12_i.dxgiGetDebugInterface1_i) = GetProcAddress(dxgiModule, "DXGIGetDebugInterface1")) != NULL;
			if (!moduleLoaded) {
				FreeLibrary(dxgiModule);
				dxgiModule = NULL;
			}
		}
		HMODULE d3d12Module = LoadLibraryW(L"D3D12.dll");
		if (d3d12Module != NULL) {
			HbBool moduleLoaded = HbTrue;
			moduleLoaded &= (*((FARPROC *) &root->api_i.d3d12_i.d3d12CreateDevice_i) = GetProcAddress(d3d12Module, "D3D12CreateDevice")) != NULL;
			moduleLoaded &= (*((FARPROC *) &root->api_i.d3d12_i.d3d12GetDebugInterface_i) = GetProcAddress(d3d12Module, "D3D12GetDebugInterface")) != NULL;
			if (!moduleLoaded) {
				FreeLibrary(d3d12Module);
				d3d12Module = NULL;
			}
		}
		if (dxgiModule != NULL && d3d12Module != NULL) {
			root->api_i.d3d12_i.dxgiModule_i = dxgiModule;
			root->api_i.d3d12_i.d3d12Module_i = d3d12Module;
			root->supportedAPIs_r |= HbGPU_Impl_APIBits_D3D12;
		} else {
			if (d3d12Module != NULL) {
				FreeLibrary(d3d12Module);
			}
			if (dxgiModule != NULL) {
				FreeLibrary(dxgiModule);
			}
		}
		#else
		// Statically linked.
		root->api_i.d3d12_i.createDXGIFactory2_i = CreateDXGIFactory2;
		root->api_i.d3d12_i.dxgiGetDebugInterface1_i = DXGIGetDebugInterface1;
		root->api_i.d3d12_i.d3d12CreateDevice_i = D3D12CreateDevice;
		root->api_i.d3d12_i.d3d12GetDebugInterface_i = D3D12GetDebugInterface;
		root->supportedAPIs_r |= HbGPU_Impl_APIBits_D3D12;
		#endif
	}
	#endif

	#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_Vulkan
	{
		#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		HMODULE const vulkanModule = LoadLibraryW(L"vulkan-1.dll");
		if (vulkanModule != NULL) {
			HbBool moduleLoaded = HbTrue;
			moduleLoaded &= (*((FARPROC *) &root->api_i.vulkan_i.vkCreateInstance_i) = GetProcAddress(vulkanModule, "vkCreateInstance")) != NULL;
			moduleLoaded &= (*((FARPROC *) &root->api_i.vulkan_i.vkGetInstanceProcAddr_i) = GetProcAddress(vulkanModule, "vkGetInstanceProcAddr")) != NULL;
			if (moduleLoaded) {
				root->api_i.vulkan_i.vulkanModule_i = vulkanModule;
				root->supportedAPIs_r |= HbGPU_Impl_APIBits_Vulkan;
			} else {
				FreeLibrary(vulkanModule);
			}
		}
		#else
		// If not dynamically loaded, assume it's statically linked.
		root->api_i.vulkan_i.vkCreateInstance_i = vkCreateInstance;
		root->api_i.vulkan_i.vkGetInstanceProcAddr_i = vkGetInstanceProcAddr;
		root->supportedAPIs_r |= HbGPU_Impl_APIBits_Vulkan;
		#endif
	}
	#endif

	if (validateUsage) {
		#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
		if (root->supportedAPIs_r & HbGPU_Impl_APIBits_D3D12) {
			ID3D12Debug * d3d12Debug;
			if (SUCCEEDED(root->api_i.d3d12_i.d3d12GetDebugInterface_i(&IID_ID3D12Debug, (IUnknown * *) &d3d12Debug))) {
				ID3D12Debug_EnableDebugLayer(d3d12Debug);
				ID3D12Debug_Release(d3d12Debug);
			}
		}
		#endif
	}
}

void HbGPU_Root_Shutdown(HbGPU_Root * const root) {
	HbReport_Assert_Assume(root != NULL);

	if (root->validateUsage_r) {
		#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
		if (root->supportedAPIs_r & HbGPU_Impl_APIBits_D3D12) {
			IDXGIDebug * dxgiDebug;
			if (SUCCEEDED(root->api_i.d3d12_i.dxgiGetDebugInterface1_i(0, &IID_IDXGIDebug, (IUnknown * *) &dxgiDebug))) {
				IDXGIDebug_ReportLiveObjects(dxgiDebug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
				IDXGIDebug_Release(dxgiDebug);
			}
		}
		#endif
	}

	#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_Vulkan
	if (root->supportedAPIs_r & HbGPU_Impl_APIBits_Vulkan) {
		HbReport_Assert_Assume(root->api_i.vulkan_i.vulkanModule_i != NULL);
		FreeLibrary(root->api_i.vulkan_i.vulkanModule_i);
	}
	#endif
	#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
	if (root->supportedAPIs_r & HbGPU_Impl_APIBits_D3D12) {
		HbReport_Assert_Assume(root->api_i.d3d12_i.d3d12Module_i != NULL);
		FreeLibrary(root->api_i.d3d12_i.d3d12Module_i);
		HbReport_Assert_Assume(root->api_i.d3d12_i.dxgiModule_i != NULL);
		FreeLibrary(root->api_i.d3d12_i.dxgiModule_i);
	}
	#endif
	#endif
}

#ifndef HbInclude_HbGPU
#define HbInclude_HbGPU
#include "HbMem.h"

#define HbGPU_Impl_API_D3D12 0
#define HbGPU_Impl_APIBits_D3D12 (1u << HbGPU_Impl_API_D3D12)

#if defined(HbPlatform_OS_Microsoft)
#define HbGPU_Impl_APIs HbGPU_Impl_APIBits_D3D12
#include <Windows.h>
#endif
#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#endif

#ifndef HbGPU_Impl_APIs
#define HbGPU_Impl_APIs 0 // May still use parts of the engine not requiring the GPU.
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned HbGPU_Impl_API;
typedef unsigned HbGPU_Impl_APIBits;

typedef struct HbGPU_Root {
	HbMem_Tag_Root * memTagRoot_e;
	HbBool validateUsage_r;
	HbGPU_Impl_APIBits supportedAPIs_r;
	#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
	struct {
		#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		HMODULE dxgiModule_i;
		HMODULE d3d12Module_i;
		HMODULE d3d12SDKLayersModule_i;
		#endif
		HRESULT (WINAPI * createDXGIFactory2_i)(UINT flags, REFIID riid, void * * factory);
		HRESULT (WINAPI * dxgiGetDebugInterface1_i)(UINT flags, REFIID riid, void * * debug);
		PFN_D3D12_CREATE_DEVICE d3d12CreateDevice_i;
		PFN_D3D12_GET_DEBUG_INTERFACE d3d12GetDebugInterface_i;
	} d3d12API_i;
	#endif
} HbGPU_Root;

void HbGPU_Root_Init(HbGPU_Root * const root, HbBool validateUsage, HbMem_Tag_Root * const memTagRoot);
void HbGPU_Root_Shutdown(HbGPU_Root * const root);

#ifdef __cplusplus
}
#endif
#endif

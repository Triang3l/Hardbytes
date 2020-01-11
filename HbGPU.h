#ifndef HbInclude_HbGPU
#define HbInclude_HbGPU
#include "HbMem.h"

#define HbGPU_Impl_API_D3D12 0
#define HbGPU_Impl_API_Vulkan (HbGPU_Impl_API_D3D12 + 1)
#define HbGPU_Impl_APIBits_D3D12 (1u << HbGPU_Impl_API_D3D12)
#define HbGPU_Impl_APIBits_Vulkan (1u << HbGPU_Impl_API_Vulkan)

#if defined(HbPlatform_OS_Microsoft)
#include <Windows.h> // Needed for winapifamily.h and SetEventOnCompletion, and CreateSwapChainForHwnd on the desktop partition.
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define HbGPU_Impl_APIs (HbGPU_Impl_APIBits_D3D12 | HbGPU_Impl_APIBits_Vulkan)
#else
#define HbGPU_Impl_APIs HbGPU_Impl_APIBits_D3D12
#endif
#endif
#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>
#endif
#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#endif
#include "Vulkan-Headers/include/vulkan/vulkan.h"
#endif

#ifndef HbGPU_Impl_APIs
#define HbGPU_Impl_APIs 0 // May still use parts of the engine not requiring the GPU.
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned HbGPU_Impl_API;
typedef unsigned HbGPU_Impl_APIBits;

/*******************
 * System API setup
 *******************/

typedef struct HbGPU_Root {
	HbMem_Tag_Root * memTagRoot_e;
	HbBool validateUsage_r;
	HbGPU_Impl_APIBits supportedAPIs_r;
	struct {
		#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_D3D12
		struct {
			#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			HMODULE dxgiModule_i;
			HMODULE d3d12Module_i;
			#endif
			HRESULT (WINAPI * createDXGIFactory2_i)(UINT flags, REFIID riid, void * * factory);
			HRESULT (WINAPI * dxgiGetDebugInterface1_i)(UINT flags, REFIID riid, void * * debug);
			PFN_D3D12_CREATE_DEVICE d3d12CreateDevice_i;
			PFN_D3D12_GET_DEBUG_INTERFACE d3d12GetDebugInterface_i;
		} d3d12_i;
		#endif
		#if HbGPU_Impl_APIs & HbGPU_Impl_APIBits_Vulkan
		struct {
			#if defined(HbPlatform_OS_Microsoft) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			HMODULE vulkanModule_i;
			#endif
			PFN_vkCreateInstance vkCreateInstance_i;
			PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_i;
		} vulkan_i;
		#endif
		#if HbGPU_Impl_APIs == 0
		HbByte unimplemented_i;
		#endif
	} api_i;
} HbGPU_Root;

void HbGPU_Root_Init(HbGPU_Root * const root, HbBool validateUsage, HbMem_Tag_Root * const memTagRoot);
void HbGPU_Root_Shutdown(HbGPU_Root * const root);

#ifdef __cplusplus
}
#endif
#endif

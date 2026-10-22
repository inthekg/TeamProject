#ifndef Engine_Define_h__
#define Engine_Define_h__

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <d3dcompiler.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

//#include <fx11/d3dx11effect.h>
#include <DirectTK/DDSTextureLoader.h>
#include <DirectTK/WICTextureLoader.h>
#include <DirectTK/SpriteBatch.h>
#include <DirectTK/SpriteFont.h>
#include <DirectTK/ScreenGrab.h>
#include <DirectTK/PrimitiveBatch.h>
#include <DirectTK/VertexTypes.h>
#include <DirectTK/Effects.h>


using namespace DirectX;

#include <PhysX/PxPhysicsAPI.h>
#include <PhysX/characterkinematic/PxCapsuleController.h>
#include <PhysX/characterkinematic/PxControllerManager.h>
#include <PhysX/characterkinematic/PxController.h>
#include <PhysX/cooking/PxCooking.h>
#include <PhysX/extensions/PxDefaultAllocator.h>

using namespace physx;

#include <JSON/json.hpp>
using json = nlohmann::ordered_json;

#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <ctime>
#include <fstream>
#include <Windows.h>
#include <process.h>
using namespace std;

#include "Engine_Enum.h"
#include "Engine_Macro.h"
#include "Engine_Struct.h"
#include "Engine_Typedef.h"
#include "Engine_Function.h"

// RTTR 용 추가
#include "Engine_Reflection.h"

namespace Engine
{
	static const _wstring			g_strTransformTag			= TEXT("Com_Transform");
	constexpr _uint					g_iNumMeshBones				= { 1024 };
	constexpr _uint					g_iNumMaxMaterals			= { 2048 };

	/* Spots (스포트 라이트 ) */
	constexpr unsigned int			g_iMaxSpots					= { 4 };
	constexpr unsigned int			g_iSpotAtlasCols			= { 2 };
	constexpr unsigned int			g_iSpotMapSize				= { 2048 };
	constexpr unsigned int			g_iSpotAtlasSize			= { g_iSpotMapSize };
	constexpr unsigned int			g_iSpotTileSize				= { g_iSpotAtlasSize / g_iSpotAtlasCols };

	/* CSM (캐스케이드 환경광 ) */
	constexpr unsigned int			g_iMaxCSM					= { 4 };
	constexpr unsigned int			g_iCSMAtlasCols				= { 2 };
	constexpr unsigned int			g_iCSMMapSize				= { 4096 };
	constexpr unsigned int			g_iCSMAtlasSize				= { g_iCSMMapSize };
	constexpr unsigned int			g_iCSMTileSize				= { g_iCSMAtlasSize / g_iCSMAtlasCols };

	/* Point */
	constexpr unsigned int			g_iMaxPoints				= { 2 };
	constexpr unsigned int			g_iPointAtlasCols			= { 3 };	
	constexpr unsigned int			g_iPointAtlasSize			= { 4096 };
	constexpr unsigned int			g_iPointTileSize			= { 1024 };

	/* FogVolume */
	constexpr unsigned int			g_iMaxFogVolumes			= { 8 };
	

	//TODO : 값 수정하기 
	//constexpr _uint					g_iEXBufferCount			= { 1024 }; // 행렬개수
	//constexpr _uint					g_iMaterialBufferCount		= { 20 }; // float 개수 

	constexpr unsigned int			g_iMaxWidth					= { 8192 };
	constexpr unsigned int			g_iMaxHeight				= { 4608 };
	constexpr unsigned int			g_iMaxTextureStack			= { 6 };

	/* Renderer 고정 Slot 리스트 */
	/* Sampler Slot. 최대 15까지*/
	constexpr _uint					LINEAR_WRAP_SAMPLER_SLOT	= 0;
	constexpr _uint					POINT_WRAP_SAMPLER_SLOT		= 1;
	constexpr _uint					LINEAR_CLAMP_SAMPLER_SLOT	= 2;

	/* Buffer Slot . 최대 13까지 */
	constexpr _uint					CAMERA_SLOT					= 0;
	constexpr _uint					LIGHT_SLOT					= 1;
	constexpr _uint					OBJECT_SLOT					= 2;
	constexpr _uint					MATERIAL_SLOT				= 3;
	constexpr _uint					EFFECT_SLOT					= 4;
	constexpr _uint					EFFECT_SLOT1				= 6;
	constexpr _uint					EFFECT_SLOT2				= 7;
	constexpr _uint					GODRAY_BLUR_SLOT			= 9;
	constexpr _uint					SHADOW_GLOBALS_SLOT			= 10;
	constexpr _uint					UI_SLOT						= 11;
	constexpr _uint					POST_GLOBALS_SLOT			= 12;
	constexpr _uint					DECAL_SLOT					= 13;
	

	/* SRV Slot.  최대 127까지 */
	//배열 개수만큼 연속적으로 슬롯이 할당됨
	//TODO : 슬롯 속성 게임에맞게 변경하기 
	constexpr _uint					SRV_ALBEDO_SLOT				= 0;
	constexpr _uint					SRV_NORMAL_SLOT				= 6;
	constexpr _uint					SRV_MASK1_SLOT				= 12;
	constexpr _uint					SRV_MASK2_SLOT				= 18;
	constexpr _uint					SRV_DETAIL_NORMAL_SLOT		= 19; //한장 
	constexpr _uint					SRV_RTV_DIFFUSE_SLOT		= 20;
	constexpr _uint					SRV_RTV_NORMAL_SLOT			= 21;
	constexpr _uint					SRV_RTV_ORMS_SLOT			= 22;
	constexpr _uint					SRV_RTV_DEPTH_SLOT			= 23;
	constexpr _uint					SRV_RTV_LIGHT_SLOT			= 24;	
	constexpr _uint					SRV_IRRADIANCE_SLOT			= 25;
	constexpr _uint					SRV_PREFILTERED_SLOT		= 26;
	constexpr _uint					SRV_BRDFLUT_SLOT			= 27;
	constexpr _uint					SRV_SSAO_SLOT				= 28;
	constexpr _uint					SRV_RTV_REFLECTION_SLOT		= 29;
	constexpr _uint					SRV_MATERIAL_BUFFER_SLOT	= 30; 
	constexpr _uint					SRV_IBL_BUFFER_SLOT			= 31;
	constexpr _uint					SRV_SPOT_SHADOW_SLOT		= 32;
	constexpr _uint					SRV_CSM_SHADOW_SLOT			= 33;
	constexpr _uint					SRV_POINT_SHADOW_SLOT		= 34;
	constexpr _uint					SRV_SHADOW_VIEW_BUFFER_SLOT = 35;
	constexpr _uint					SRV_TUNING_BUFFER_SLOT		= 36;
	constexpr _uint					SRV_FOGVOLUME_BUFFER_SLOT	= 37;
	constexpr _uint					SRV_EMSSIVE_SLOT			= 38;
	constexpr _uint					SRV_RTV_EMSSIVE_SLOT		= 39;
	constexpr _uint					SRV_RIMPROFILE_BUFFER_SLOT	= 40;

	// Animation CS
	constexpr _uint					SRV_BONE_MATRIX_SLOT		= 41;

	constexpr _uint					SRV_PLANAR_REFLECTION_SLOT = 42;

	/* SHADOW BASE SLOT COUNT*/
	constexpr _uint					SHADOW_VIEW_COUNT			= 20;
	constexpr _uint					SHADOW_SPOT_BASE			= 0;
	constexpr _uint					SHADOW_CSM_BASE				= 4;
	constexpr _uint					SHADOW_POINT_BASE			= 8;

	/* Bloom Mip Pyramid Level */
	constexpr _uint					BLOOM_MIP_COUNT				= 5;
}

namespace GlobalFileRoute
{
	static const wstring			g_strMaterialFolder			= { L"Materials/" };
	static const wstring			g_strTextureFolder			= { L"/Textures/" };


	static const wstring			g_strMaterialFileTag		= { L".json" };
	static const wstring			g_strMeshFileTag			= { L"_Mesh.data" };
	static const wstring			g_strPureMeshFileTag		= { L"_PureMesh.data" };
	static const wstring			g_strCookingMeshFileTag		= { L"_CookingMesh.data" };
	static const wstring			g_strBoneMeshFileTag		= { L"_Bone+Mesh.data" };
	static const wstring			g_strAnimationFileTag		= { L"_Animation.anim" };
	static const wstring			g_strAnimationCSVFileTag	= { L"_AnimationData.csv" };
	
	// 추가
	static const wstring			g_strAnimSettingFileTag		= { L"_AnimClipSettings.json" };
}

namespace PhysX
{
	static constexpr float			g_fGravity					= { -9.81f };
	static constexpr float			g_fGravityScale				= { 5.f };

}

#define TEXTURE_TYPE_MAX  ETOUI(TEXTURE_TYPE::END)
#define MAX_PATH_LEN 60
#define MAX_NAME_LEN 200

#define DIRECTINPUT_VERSION	0x0800

#pragma warning(disable : 4251)

#ifdef _DEBUG

	#define IMGUI_DEFINE_MATH_OPERATORS
	#include "imgui.h"


	#include <iostream>
	#include <cstdio>
	#include <cstdarg>

	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>


	#ifndef DBG_NEW 

	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
	//#define new DBG_NEW 

	#endif

#endif


using namespace Engine;

#endif // Engine_Define_h__

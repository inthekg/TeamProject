#pragma once

#include "Core_Defines.h"

NS_BEGIN(GameCore)

enum class CUTSCENE_PLAY_STATE
{
	STOPPED,
	PLAYING,
	PAUSED,
	END
};

enum class CUTSCENE_OBJECT_TYPE
{
	STATIC_PROP,
	ANIMATED_ACTOR,
	RUNTIME_PLAYER,
	END
};

enum class CUTSCENE_EFFECT_ATTACH_TYPE
{
	WORLD,       // 고정 월드 위치에서 재생한다.
	OBJECT,      // 한 오브젝트의 World Transform을 따라간다.
	BONE,        // 한 스킨드 모델의 한 Bone을 따라간다.
	BONE_PAIR,   // 한 스킨드 모델의 두 Bone 사이를 사용한다.
	MODEL,       // 한 스킨드 모델 전체를 Effect Model 입력으로 사용한다.
	PARTS,       // 여러 컷신 액터의 Model을 Parts 입력으로 사용한다.
	OBJECT_PAIR, // 서로 다른 두 오브젝트의 World Transform을 사용한다.
	END
};

enum class CUTSCENE_NOTIFY_ACTION_TYPE
{
	SOUND,
	EFFECT,
	CAMERA_SHAKE,
	END
};

struct CUTSCENE_TRANSFORM_VALUE
{
	_float3 vScale = { 1.f, 1.f, 1.f };
	_float3 vRotationDegree = {};
	_float3 vPosition = {};
};

struct CUTSCENE_TRANSFORM_KEY
{
	_float fTime = {};
	CUTSCENE_TRANSFORM_VALUE Transform = {};
};

struct CUTSCENE_FLOAT_KEY
{
	_float fTime = {};
	_float fValue = {};
};

struct CUTSCENE_DISSOLVE_KEY
{
	_float fTime = {};
	// 엔진 렌더 기준으로 0은 원본 표시, 1은 완전 디졸브다.
	_float fAmount = {};
	// RGB는 경계색, W는 경계 발광 세기다.
	_float4 vColor = { 0.05f, 0.45f, 1.f, 6.f };
};

struct CUTSCENE_CAMERA_KEY
{
	// Camera Track 내부의 로컬 시간이다.
	_float fTime = {};
	_float3 vPosition = {};
	_float4 vQuaternion = { 0.f, 0.f, 0.f, 1.f };
	// CCamera::Set_Fovy와 동일하게 세로 FOV Radian을 저장한다.
	_float fVerticalFovy = { XMConvertToRadians(60.f) };
};

struct CUTSCENE_CAMERA_TRACK
{
	_uint iTrackID = { UINT_MAX };
	string strTrackName = {};
	vector<CUTSCENE_CAMERA_KEY> Keys = {};
};

struct CUTSCENE_CAMERA_SCENE_SECTION
{
	_uint iSectionID = { UINT_MAX };
	_uint iCameraTrackID = { UINT_MAX };
	_float fStartTime = {};
	_float fEndTime = {};
	// 기존 파일과의 호환 및 에디터 표시용 이름이다.
	string strSceneKey = {};
	_float fBlendIn = { 0.2f };
	_float fBlendOut = { 0.2f };
	_float fPlayRate = { 1.f };
	_float3 vPreviewPosition = {};
	_float3 vPreviewDirection = { 0.f, 0.f, 1.f };
	_bool bShowPreviewGuide = { true };
};

struct CUTSCENE_CAMERA_SHAKE_EVENT
{
	_uint iEventID = { UINT_MAX };
	_float fTime = {};
	string strShakeKey = {};
};

struct CUTSCENE_SCREEN_KEY
{
	_float fTime = {};
	_float fFadeAmount = {};
	_float4 vColor = { 0.f, 0.f, 0.f, 1.f };
	_float fColorAmount = {};
};

struct CUTSCENE_ANIMATION_SECTION
{
	_uint iSectionID = { UINT_MAX };
	_float fStartTime = {};
	_float fEndTime = {};
	_wstring strClipName = {};
	_float fClipStartTime = {};
	_float fClipEndTime = {};
	_float fPlayRate = { 1.f };
	// 이전 Animation Section의 마지막 Pose에서 이 Section의 시작 Pose로 보간하는 시간(초)이다.
	_float fBlendInTime = {};
	_bool bLoop = { false };
	_bool bUseRootMotion = { true };
	// 독립 컷신 소품이 새 Clip의 Root Motion을 자기 기준 위치부터 시작할 때만 사용한다.
	_bool bResetRootMotion = { false };
	// 원본 애니메이션에 포함된 축/원점 차이를 게임 좌표계 기준 local Transform으로 보정한다.
	_float3 vPositionCorrection = {};
	_float3 vRotationCorrectionDegree = {};
};

struct CUTSCENE_INSTANCE_SOURCE
{
	_wstring strMapName = {};
	_uint iGroupIndex = { UINT_MAX };
	_uint iInstanceIndex = { UINT_MAX };
	// true면 GroupIndex를 Map의 정적 오브젝트 인덱스로 해석하고 InstanceIndex는 사용하지 않는다.
	_bool bStaticObject = { false };

	_bool Is_Valid() const
	{
		return false == strMapName.empty() && UINT_MAX != iGroupIndex && (bStaticObject || UINT_MAX != iInstanceIndex);
	}
};

struct CUTSCENE_VISIBILITY_KEY
{
	_float fTime = {};
	_bool bVisible = { true };
};

struct CUTSCENE_MESH_VISIBILITY_KEY
{
	_float fTime = {};
	_uint iMeshIndex = {};
	_bool bVisible = { true };
};

struct CUTSCENE_ATTACHMENT_KEY
{
	_float fTime = {};
	_bool bAttached = { true };
	_uint iParentObjectID = { UINT_MAX };
	// 부모의 이 Bone과 자식 모델의 Child Bone을 서로 겹친다. 비어 있으면 자식 모델 원점을 사용한다.
	string strAttachBoneName = {};
	string strChildAttachBoneName = {};
	// 부모 또는 Bone 전환 시 이전 World 위치에서 새 부착 위치까지 이동하는 시간이다.
	_float fBlendTime = {};
	// false이면 부모 Bone의 위치만 따라가고 분기 직전의 World 회전은 유지한다.
	_bool bInheritParentRotation = { true };
	// 회전 상속을 다시 켰을 때 잠긴 World 회전에서 부모 Bone 회전까지 복귀하는 시간이다.
	_float fParentRotationBlendTime = {};
};

struct CUTSCENE_OBJECT_TRACK
{
	_uint iObjectID = { UINT_MAX };
	_wstring strObjectName = {};
	CUTSCENE_OBJECT_TYPE eObjectType = { CUTSCENE_OBJECT_TYPE::END };
	_float fSpawnTime = {};
	_float fDespawnTime = { FLT_MAX };

	_wstring strMeshName = {};
	_wstring strMaterialName = {};

	_wstring strModelResourceName = {};
	_wstring strModelPrototypeTag = {};
	_uint iModelLevel = {};
	// 모델 기반 컷신 소품도 사용할 수 있도록 GPU Skinning 사용 여부를 Track에 명시한다.
	_bool bSkinnedModel = { true };
	_wstring strAnimFolderTag = {};
	_wstring strAnimTag = {};
	string strRootBoneName = { "Root" };
	_uint iParentObjectID = { UINT_MAX };
	string strAttachBoneName = {};
	// Bone이 없는 정적 모델은 비워 두고 모델 원점을 부모 Bone에 부착한다.
	string strChildAttachBoneName = {};

	CUTSCENE_TRANSFORM_VALUE InitialTransform = {};
	CUTSCENE_INSTANCE_SOURCE InstanceSource = {};
	vector<CUTSCENE_TRANSFORM_KEY> TransformKeys = {};
	vector<CUTSCENE_FLOAT_KEY> EmissiveKeys = {};
	vector<CUTSCENE_DISSOLVE_KEY> DissolveKeys = {};
	vector<CUTSCENE_VISIBILITY_KEY> VisibilityKeys = {};
	vector<CUTSCENE_MESH_VISIBILITY_KEY> MeshVisibilityKeys = {};
	vector<CUTSCENE_ATTACHMENT_KEY> AttachmentKeys = {};
	vector<CUTSCENE_ANIMATION_SECTION> AnimationSections = {};
};

struct CUTSCENE_LIGHT_KEY
{
	_float fTime = {};
	_bool bEnable = { true };
	_float fIntensity = { 1.f };
	_float4 vDiffuse = { 1.f, 1.f, 1.f, 1.f };
	_float4 vAmbient = { 0.f, 0.f, 0.f, 0.2f };
	_float4 vSpecular = { 1.f, 1.f, 1.f, 1.f };
};

struct CUTSCENE_LIGHT_TRACK
{
	_uint iLightID = { UINT_MAX };
	_bool bSpawnForCutScene = { false };
	LIGHT eLightType = { LIGHT::POINT };
	_float fSpawnTime = {};
	_float fDespawnTime = { FLT_MAX };
	CUTSCENE_TRANSFORM_VALUE InitialTransform = {};
	_float fRange = { 10.f };
	_float fInnerDegree = { 20.f };
	_float fOuterDegree = { 30.f };
	_bool bShadow = { true };
	_bool bGodRay = { false };
	_float fGodRayIntensity = { 1.f };
	vector<CUTSCENE_TRANSFORM_KEY> TransformKeys = {};
	vector<CUTSCENE_LIGHT_KEY> Keys = {};
};

struct CUTSCENE_SOUND_EVENT
{
	_uint iEventID = { UINT_MAX };
	_float fStartTime = {};
	_float fEndTime = {};
	string strSoundKey = {};
	CHANNEL_ID eChannel = { SOUND_SFX };
	_float fVolume = { 1.f };
	_bool bLoop = { false };
	_bool bBGM = { false };
	_bool bUseWorldPosition = { false };
	_uint iObjectID = { UINT_MAX };
	_float3 vWorldPosition = {};
};

struct CUTSCENE_EFFECT_EVENT
{
	_uint iEventID = { UINT_MAX };
	_float fStartTime = {};
	_float fEndTime = {};
	string strEffectKey = {};
	CUTSCENE_EFFECT_ATTACH_TYPE eAttachType = { CUTSCENE_EFFECT_ATTACH_TYPE::WORLD };
	// iObjectID는 기본 대상 또는 Parts Effect의 Owner Transform 대상이다.
	_uint iObjectID = { UINT_MAX };
	// 두 번째 오브젝트/Bone 및 Parts 대상은 해당 Attach Type에서만 사용한다.
	_uint iSecondaryObjectID = { UINT_MAX };
	string strBoneName = {};
	string strSecondaryBoneName = {};
	vector<_uint> PartObjectIDs = {};
	_float3 vWorldPosition = {};
	_float3 vOffset = {};
	// World 위치 전용 overload를 제외한 Effect 회전 보정값이다.
	_float3 vAngle = {};
	_bool bLoop = { false };
};

struct CUTSCENE_NOTIFY_BINDING
{
	_uint iObjectID = { UINT_MAX };
	_uint64 iNotifyHash = {};
	CUTSCENE_NOTIFY_ACTION_TYPE eActionType = { CUTSCENE_NOTIFY_ACTION_TYPE::END };
	string strResourceKey = {};
	string strBoneName = {};
	CHANNEL_ID eSoundChannel = { SOUND_SFX };
	_float fVolume = { 1.f };
	_float3 vOffset = {};
};

struct CUTSCENE_ASSET
{
	_uint iVersion = { 16 };
	string strName = { "NewCutScene" };
	_float fDuration = { 10.f };
	_float fFramesPerSecond = { 60.f };

	vector<CUTSCENE_CAMERA_TRACK> CameraTracks = {};
	vector<CUTSCENE_CAMERA_SCENE_SECTION> CameraSceneSections = {};
	vector<CUTSCENE_CAMERA_SHAKE_EVENT> CameraShakeEvents = {};
	vector<CUTSCENE_SCREEN_KEY> ScreenKeys = {};
	vector<CUTSCENE_OBJECT_TRACK> ObjectTracks = {};
	vector<CUTSCENE_LIGHT_TRACK> LightTracks = {};
	vector<CUTSCENE_SOUND_EVENT> SoundEvents = {};
	vector<CUTSCENE_EFFECT_EVENT> EffectEvents = {};
	vector<CUTSCENE_NOTIFY_BINDING> NotifyBindings = {};
};

NS_END

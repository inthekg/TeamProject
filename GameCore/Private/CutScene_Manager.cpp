#include "CutScene_Manager.h"

#include "AnimDriver.h"
#include "Camera.h"
#include "CCT.h"
#include "CutScene_Actor.h"
#include "CutScene_Camera.h"
#include "CutScene_Prop.h"
#include "Effect_ResourceCatalog.h"
#include "CutScene_Serializer.h"
#include "Engine_Utility.h"
#include "Effect.h"
#include "GameInstance.h"
#include "Map.h"
#include "MapInstanceObject.h"
#include "MapObject.h"
#include "Player.h"
#include "UIElement.h"
#include "UIObject.h"

static _float CutScene_RuntimeClamp01(_float fValue)
{
	return max(0.f, min(fValue, 1.f));
}

static _float CutScene_RuntimeKeyRatio(_float fTime, _float fLeftTime, _float fRightTime)
{
	const _float fDuration = fRightTime - fLeftTime;

	if (fDuration <= 0.000001f)
		return 0.f;

	return CutScene_RuntimeClamp01((fTime - fLeftTime) / fDuration);
}

static _bool CutScene_RuntimeSampleCamera(const GameCore::CUTSCENE_CAMERA_TRACK& Track, _float fTime, GameCore::CUTSCENE_CAMERA_KEY* pOutKey)
{
	if (nullptr == pOutKey || Track.Keys.empty())
		return false;

	if (fTime <= Track.Keys.front().fTime)
	{
		*pOutKey = Track.Keys.front();
		return true;
	}

	if (fTime >= Track.Keys.back().fTime)
	{
		*pOutKey = Track.Keys.back();
		return true;
	}

	const GameCore::CUTSCENE_CAMERA_KEY* pLeft = &Track.Keys.front();
	const GameCore::CUTSCENE_CAMERA_KEY* pRight = pLeft;

	for (_uint i = 1; i < static_cast<_uint>(Track.Keys.size()); ++i)
	{
		if (fTime <= Track.Keys[i].fTime)
		{
			pLeft = &Track.Keys[i - 1];
			pRight = &Track.Keys[i];
			break;
		}
	}

	const _float fRatio = CutScene_RuntimeKeyRatio(fTime, pLeft->fTime, pRight->fTime);
	pOutKey->fTime = fTime;
	XMStoreFloat3(&pOutKey->vPosition, XMVectorLerp(XMLoadFloat3(&pLeft->vPosition), XMLoadFloat3(&pRight->vPosition), fRatio));
	pOutKey->fVerticalFovy = pLeft->fVerticalFovy + (pRight->fVerticalFovy - pLeft->fVerticalFovy) * fRatio;

	_vector vLeftQuaternion = XMQuaternionNormalize(XMLoadFloat4(&pLeft->vQuaternion));
	_vector vRightQuaternion = XMQuaternionNormalize(XMLoadFloat4(&pRight->vQuaternion));

	// 같은 회전을 나타내는 Quaternion 부호가 달라도 가장 짧은 회전 경로로 보간한다.
	if (XMVectorGetX(XMVector4Dot(vLeftQuaternion, vRightQuaternion)) < 0.f)
		vRightQuaternion = XMVectorNegate(vRightQuaternion);

	XMStoreFloat4(&pOutKey->vQuaternion, XMQuaternionNormalize(XMQuaternionSlerp(vLeftQuaternion, vRightQuaternion, fRatio)));
	return true;
}

static GameCore::CUTSCENE_TRANSFORM_VALUE CutScene_RuntimeSampleTransform(const vector<GameCore::CUTSCENE_TRANSFORM_KEY>& Keys, const GameCore::CUTSCENE_TRANSFORM_VALUE& DefaultValue, _float fTime)
{
	if (Keys.empty())
		return DefaultValue;

	if (fTime <= Keys.front().fTime)
		return Keys.front().Transform;

	if (fTime >= Keys.back().fTime)
		return Keys.back().Transform;

	for (_uint i = 1; i < static_cast<_uint>(Keys.size()); ++i)
	{
		if (fTime >= Keys[i].fTime)
			continue;

		const _float fRatio = CutScene_RuntimeKeyRatio(fTime, Keys[i - 1].fTime, Keys[i].fTime);
		GameCore::CUTSCENE_TRANSFORM_VALUE Result = {};
		XMStoreFloat3(&Result.vScale, XMVectorLerp(XMLoadFloat3(&Keys[i - 1].Transform.vScale), XMLoadFloat3(&Keys[i].Transform.vScale), fRatio));
		XMStoreFloat3(&Result.vRotationDegree, XMVectorLerp(XMLoadFloat3(&Keys[i - 1].Transform.vRotationDegree), XMLoadFloat3(&Keys[i].Transform.vRotationDegree), fRatio));
		// 부착 오브젝트의 Local Offset은 다음 키까지 유지해 부모 Bone 기준으로 미끄러지지 않게 한다.
		Result.vPosition = Keys[i - 1].Transform.vPosition;
		return Result;
	}

	return Keys.back().Transform;
}

static const GameCore::CUTSCENE_ANIMATION_SECTION* CutScene_RuntimeFindAnimationSection(const vector<GameCore::CUTSCENE_ANIMATION_SECTION>& Sections, _float fTime)
{
	for (const GameCore::CUTSCENE_ANIMATION_SECTION& Section : Sections)
	{
		if (Section.fStartTime > fTime)
			break;

		if (fTime >= Section.fStartTime && fTime < Section.fEndTime)
			return &Section;
	}

	return nullptr;
}

static _float CutScene_RuntimeSampleFloat(const vector<GameCore::CUTSCENE_FLOAT_KEY>& Keys, _float fDefaultValue, _float fTime)
{
	if (Keys.empty())
		return fDefaultValue;

	if (fTime <= Keys.front().fTime)
		return Keys.front().fValue;

	if (fTime >= Keys.back().fTime)
		return Keys.back().fValue;

	for (_uint i = 1; i < static_cast<_uint>(Keys.size()); ++i)
	{
		if (fTime > Keys[i].fTime)
			continue;

		const _float fRatio = CutScene_RuntimeKeyRatio(fTime, Keys[i - 1].fTime, Keys[i].fTime);
		return Keys[i - 1].fValue + (Keys[i].fValue - Keys[i - 1].fValue) * fRatio;
	}

	return Keys.back().fValue;
}

static GameCore::CUTSCENE_DISSOLVE_KEY CutScene_RuntimeSampleDissolve(const vector<GameCore::CUTSCENE_DISSOLVE_KEY>& Keys, _float fTime)
{
	GameCore::CUTSCENE_DISSOLVE_KEY Result = {};

	if (Keys.empty())
		return Result;

	// 첫 키 전에는 원본을 유지하되, 첫 키에 지정한 경계색은 미리 사용한다.
	Result.vColor = Keys.front().vColor;
	if (fTime < Keys.front().fTime)
		return Result;

	if (fTime >= Keys.back().fTime)
		return Keys.back();

	for (_uint i = 1; i < static_cast<_uint>(Keys.size()); ++i)
	{
		if (fTime > Keys[i].fTime)
			continue;

		const GameCore::CUTSCENE_DISSOLVE_KEY& Left = Keys[i - 1];
		const GameCore::CUTSCENE_DISSOLVE_KEY& Right = Keys[i];
		const _float fRatio = max(0.f, min((fTime - Left.fTime) / max(Right.fTime - Left.fTime, FLT_EPSILON), 1.f));
		Result.fTime = fTime;
		Result.fAmount = Left.fAmount + (Right.fAmount - Left.fAmount) * fRatio;
		XMStoreFloat4(&Result.vColor, XMVectorLerp(XMLoadFloat4(&Left.vColor), XMLoadFloat4(&Right.vColor), fRatio));
		return Result;
	}

	return Keys.back();
}

static _bool CutScene_RuntimeSampleVisibility(const vector<GameCore::CUTSCENE_VISIBILITY_KEY>& Keys, _bool bDefaultValue, _float fTime)
{
	_bool bResult = bDefaultValue;

	// Visibility는 보간하지 않고 현재 시간 이전의 마지막 키를 즉시 적용한다.
	for (const GameCore::CUTSCENE_VISIBILITY_KEY& Key : Keys)
	{
		if (Key.fTime > fTime)
			break;

		bResult = Key.bVisible;
	}

	return bResult;
}

static GameCore::CUTSCENE_ATTACHMENT_KEY CutScene_RuntimeSampleAttachment(const GameCore::CUTSCENE_OBJECT_TRACK& Track, _float fTime)
{
	GameCore::CUTSCENE_ATTACHMENT_KEY Result = {};
	Result.bAttached = UINT_MAX != Track.iParentObjectID && false == Track.strAttachBoneName.empty();
	Result.iParentObjectID = Track.iParentObjectID;
	Result.strAttachBoneName = Track.strAttachBoneName;
	Result.strChildAttachBoneName = Track.strChildAttachBoneName;

	// 부착 상태와 대상 본은 보간하지 않고 현재 시간 이전의 마지막 키를 즉시 적용한다.
	for (const GameCore::CUTSCENE_ATTACHMENT_KEY& Key : Track.AttachmentKeys)
	{
		if (Key.fTime > fTime)
			break;

		Result = Key;
	}

	if (false == Result.bAttached || UINT_MAX == Result.iParentObjectID || Result.strAttachBoneName.empty())
		Result.bAttached = false;

	return Result;
}

static _float CutScene_RuntimeGetAttachmentBlendWeight(const GameCore::CUTSCENE_ATTACHMENT_KEY& Key, _float fTime, _bool bSeek)
{
	if (bSeek || Key.fBlendTime <= 0.f)
		return 1.f;

	return max(0.f, min((fTime - Key.fTime) / Key.fBlendTime, 1.f));
}

static _float CutScene_RuntimeGetParentRotationWeight(const GameCore::CUTSCENE_ATTACHMENT_KEY& Key, _float fTime)
{
	if (false == Key.bInheritParentRotation)
		return 0.f;

	if (Key.fParentRotationBlendTime <= 0.f)
		return 1.f;

	// Seek와 연속 재생 모두 키 시간 기준의 동일한 회전 복귀 비율을 사용한다.
	return max(0.f, min((fTime - Key.fTime) / Key.fParentRotationBlendTime, 1.f));
}

static _uint CutScene_RuntimeGetObjectHierarchyDepth(const vector<GameCore::CUTSCENE_OBJECT_TRACK>& Tracks, const GameCore::CUTSCENE_OBJECT_TRACK& Track)
{
	_uint iDepth = {};
	_uint iParentObjectID = Track.iParentObjectID;
	unordered_set<_uint> VisitedObjectIDs = {};

	while (UINT_MAX != iParentObjectID && VisitedObjectIDs.emplace(iParentObjectID).second)
	{
		const GameCore::CUTSCENE_OBJECT_TRACK* pParentTrack = nullptr;

		for (const GameCore::CUTSCENE_OBJECT_TRACK& Candidate : Tracks)
		{
			if (Candidate.iObjectID == iParentObjectID)
			{
				pParentTrack = &Candidate;
				break;
			}
		}

		if (nullptr == pParentTrack)
			break;

		++iDepth;
		iParentObjectID = pParentTrack->iParentObjectID;
	}

	return iDepth;
}

static _float4x4 CutScene_RuntimeMakeWorldMatrix(const GameCore::CUTSCENE_TRANSFORM_VALUE& Transform)
{
	_float4x4 WorldMatrix = {};
	const _matrix Matrix = XMMatrixScaling(Transform.vScale.x, Transform.vScale.y, Transform.vScale.z) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(Transform.vRotationDegree.x), XMConvertToRadians(Transform.vRotationDegree.y), XMConvertToRadians(Transform.vRotationDegree.z)) * XMMatrixTranslation(Transform.vPosition.x, Transform.vPosition.y, Transform.vPosition.z);
	XMStoreFloat4x4(&WorldMatrix, Matrix);
	return WorldMatrix;
}

CCutScene_Manager::CCutScene_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }, m_pContext{ pContext }, m_pGameInstance{ CGameInstance::Get_Instance() }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CCutScene_Manager::Initialize(const function<_bool()>& CanPlayCallback, const function<void(_bool)>& PlaybackStateCallback, const function<_bool(const string&)>& PlaybackFinishedCallback)
{
	if (nullptr == m_pGameInstance || false == static_cast<_bool>(CanPlayCallback) || false == static_cast<_bool>(PlaybackStateCallback) || false == static_cast<_bool>(PlaybackFinishedCallback))
		return E_FAIL;

	m_CanPlayCallback = CanPlayCallback;
	m_PlaybackStateCallback = PlaybackStateCallback;
	m_PlaybackFinishedCallback = PlaybackFinishedCallback;

	if (FAILED(Ready_RuntimeCamera()))
		return E_FAIL;

	if (FAILED(Ready_ScreenOverlay()))
		return E_FAIL;

	// 컷신 요청은 Level을 경유하지 않고 컷신 매니저가 직접 소유한다.
	m_pGameInstance->Subscribe(EVENT_TYPE::PLAY_CUTSCENE, this);
	m_pGameInstance->Subscribe(EVENT_TYPE::STOP_CUTSCENE, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_FESTIVAL, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_HOTEL, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_BRIDGE, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P1, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P2, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_DEAD, this);
	m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_SOPHIA_DEAD, this);

	return S_OK;
}

void CCutScene_Manager::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	switch (eType)
	{
	case EVENT_TYPE::PLAY_CUTSCENE:
		Handle_PlayRequest(pData, false);
		break;
	case EVENT_TYPE::STOP_CUTSCENE:
		Stop();
		break;
	}
}

void CCutScene_Manager::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	switch (eType)
	{
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_FESTIVAL:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_HOTEL:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_BRIDGE:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P1:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_P2:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_RAXASIA_DEAD:
	case EVENT_TYPE_MAP_OBJECT::START_CUTSCENE_SOPHIA_DEAD:
		Handle_PlayRequest(pData, true);
		break;
	}
}

void CCutScene_Manager::Handle_PlayRequest(Event_Desc* pData, _bool bInteractionRequest)
{
	if (nullptr == pData)
		return;

	// Broadcast는 동기 호출이므로 요청자는 같은 프레임에 재생 성공 여부를 확인할 수 있다.
	CUTSCENE_REQUEST* pRequest = static_cast<CUTSCENE_REQUEST*>(pData);
	pRequest->hResult = Play(pRequest->strAssetName);

	if (bInteractionRequest && FAILED(pRequest->hResult))
		OutputDebugStringA(("[Interaction CutScene Error] SpawnID " + to_string(pRequest->iTargetSpawnID) + " / " + pRequest->strAssetName + "\n").c_str());
}

void CCutScene_Manager::Set_PlayState(CUTSCENE_PLAY_STATE ePlayState)
{
	if (m_ePlayState == ePlayState)
		return;

	m_ePlayState = ePlayState;

	// 실제 Player 참조는 Level이 소유하고, 매니저는 재생 상태만 통지한다.
	if (m_PlaybackStateCallback)
		m_PlaybackStateCallback(CUTSCENE_PLAY_STATE::PLAYING == m_ePlayState);
}

void CCutScene_Manager::Suppress_HUDLayer()
{
	if (m_bHUDLayerSuppressed || nullptr == m_pGameInstance)
		return;

	// 컷신이 재생되는 동안 일반 게임 HUD만 숨긴다. 컷신 화면 Overlay는 별도 레이어라 유지된다.
	m_pGameInstance->Acquire_UILayerSuppression("HUD");
	m_bHUDLayerSuppressed = true;
}

void CCutScene_Manager::Restore_HUDLayer()
{
	if (false == m_bHUDLayerSuppressed)
		return;

	// 이 매니저가 획득한 억제 횟수만 반환해 다른 UI의 HUD 억제 상태는 보존한다.
	if (nullptr != m_pGameInstance)
		m_pGameInstance->Release_UILayerSuppression("HUD");

	m_bHUDLayerSuppressed = false;
}

HRESULT CCutScene_Manager::Play(const CUTSCENE_ASSET& Asset)
{
	return Start_Play(Asset, Asset.strName);
}

HRESULT CCutScene_Manager::Start_Play(const CUTSCENE_ASSET& Asset, const string& strAssetName)
{
	// 챕터 전환 중에는 카메라와 화면 전환이 겹치지 않도록 재생을 거부한다.
	if (m_CanPlayCallback && false == m_CanPlayCallback())
		return E_FAIL;

	if (Asset.fDuration <= 0.f || Asset.fFramesPerSecond <= 0.f)
		return E_FAIL;

	// 이전 컷신이 남긴 카메라, 화면, 오브젝트 상태를 먼저 정리한다.
	Stop();
	m_Asset = Asset;
	m_strActiveAssetName = strAssetName;

	// 오브젝트 Track이 있는 컷신은 챕터 진입 때 준비된 객체 집합이 없으면 불완전 재생하지 않는다.
	if (false == m_Asset.ObjectTracks.empty() && nullptr == Find_ActivePreparedAsset())
	{
		m_strActiveAssetName.clear();
		return E_FAIL;
	}

	Sort_CutScene();
	m_dPlaybackTime = 0.0;
	m_fCurrentTime = 0.f;
	Suppress_HUDLayer();
	Set_PlayState(CUTSCENE_PLAY_STATE::PLAYING);
	m_bPlaybackCleanupPending = false;

	if (m_hScreenOverlay.Is_Valid())
		m_pGameInstance->Open_UIObject(m_hScreenOverlay);

	Evaluate_Camera(0.f);
	Evaluate_Screen(0.f);
	Evaluate_Objects(0.f, true);
	Dispatch_Sounds(-0.0001f, 0.f);
	Dispatch_Effects(-0.0001f, 0.f);
	return S_OK;
}

HRESULT CCutScene_Manager::Preload(const string& strAssetName)
{
	if (m_CachedAssets.end() != m_CachedAssets.find(strAssetName))
		return S_OK;

	CUTSCENE_ASSET Asset = {};

	if (FAILED(Load_Asset(strAssetName, &Asset)))
		return E_FAIL;

	// Asset을 캐시에 넣기 전에 Effect 키와 원본 JSON을 한 번 연결한다.
	if (FAILED(Prepare_EffectResources(Asset)))
		return E_FAIL;

	auto Result = m_CachedAssets.emplace(strAssetName, move(Asset));

	// 챕터 준비가 끝난 뒤 추가 Preload가 발생한 경우에도 재생 시 동적 생성하지 않도록 즉시 준비한다.
	if (UINT_MAX != m_iObjectLevel && UINT_MAX != m_iModelLevel && FAILED(Prepare_Asset(strAssetName, Result.first->second)))
	{
		Release_PreparedAssets();
		m_CachedAssets.erase(strAssetName);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CCutScene_Manager::Prepare_Chapter(_uint iObjectLevel, _uint iModelLevel)
{
	Release_PreparedAssets();
	m_iObjectLevel = iObjectLevel;
	m_iModelLevel = iModelLevel;

	for (const pair<const string, CUTSCENE_ASSET>& CachedAsset : m_CachedAssets)
	{
		if (FAILED(Prepare_Asset(CachedAsset.first, CachedAsset.second)))
		{
			Release_PreparedAssets();
			return E_FAIL;
		}
	}

	return S_OK;
}

void CCutScene_Manager::Release_Chapter()
{
	Stop();
	Release_PreparedAssets();
}

HRESULT CCutScene_Manager::Play(const string& strAssetName)
{
	// 전환 중 요청은 파일 로드 전에 차단해 불필요한 동기 I/O도 발생시키지 않는다.
	if (m_CanPlayCallback && false == m_CanPlayCallback())
		return E_FAIL;

	if (FAILED(Preload(strAssetName)))
		return E_FAIL;

	const auto iter = m_CachedAssets.find(strAssetName);

	if (m_CachedAssets.end() == iter)
		return E_FAIL;

	return Start_Play(iter->second, strAssetName);
}

void CCutScene_Manager::Update(_float fTimeDelta)
{
	if (m_bPlaybackCleanupPending)
	{
		// 마지막 검은 프레임을 한 번 출력한 다음 페이드와 컷신 카메라를 함께 해제한다.
		Finish_Playback();
		return;
	}

	if (CUTSCENE_PLAY_STATE::PLAYING != m_ePlayState)
		return;

	const _float fPreviousTime = m_fCurrentTime;

	// 입력 DeltaTime은 float이지만 누적 결과는 double로 보존해 Camera Cut 경계의 반복 반올림 오차를 막는다.
	m_dPlaybackTime = min(m_dPlaybackTime + static_cast<_double>(max(fTimeDelta, 0.f)), static_cast<_double>(m_Asset.fDuration));
	m_fCurrentTime = static_cast<_float>(m_dPlaybackTime);
	Evaluate_Camera(m_fCurrentTime);
	Evaluate_Screen(m_fCurrentTime);
	Evaluate_Objects(m_fCurrentTime, false);
	Dispatch_Sounds(fPreviousTime, m_fCurrentTime);
	Dispatch_Effects(fPreviousTime, m_fCurrentTime);

	if (m_fCurrentTime >= m_Asset.fDuration)
		m_bPlaybackCleanupPending = true;
}

void CCutScene_Manager::Pre_MapUpdate()
{
	Restore_InstanceOverrides();

	if (CUTSCENE_PLAY_STATE::PLAYING != m_ePlayState)
		return;

	CUTSCENE_PREPARED_ASSET* pPreparedAsset = Find_ActivePreparedAsset();
	if (nullptr == pPreparedAsset)
		return;

	for (const CUTSCENE_OBJECT_TRACK& Track : m_Asset.ObjectTracks)
	{
		if (false == Track.InstanceSource.Is_Valid() || m_fCurrentTime < Track.fSpawnTime || m_fCurrentTime >= Track.fDespawnTime)
			continue;

		auto RuntimeIter = pPreparedAsset->Objects.find(Track.iObjectID);
		if (pPreparedAsset->Objects.end() == RuntimeIter)
			continue;

		if (Track.InstanceSource.bStaticObject)
		{
			// 정적 맵 파트는 원본 한 개만 컷신 대체 소품이 렌더되는 동안 제외한다.
			RuntimeIter->second.pStaticSource = Resolve_StaticSource(Track.InstanceSource);
			if (nullptr != RuntimeIter->second.pStaticSource && false == RuntimeIter->second.pStaticSource->Is_InActive())
			{
				RuntimeIter->second.pStaticSource->Set_InActive();
				m_TemporarilyHiddenInstances.push_back(RuntimeIter->second.pStaticSource);
			}
			continue;
		}

		// 맵 재로드 뒤에도 안정 ID로 원본 Instance owner를 다시 찾는다.
		RuntimeIter->second.pInstanceOwner = Resolve_InstanceOwner(Track.InstanceSource);
		if (nullptr == RuntimeIter->second.pInstanceOwner)
			continue;

		CMapObject* pInstanceObject = RuntimeIter->second.pInstanceOwner->Get_InstanceObject(Track.InstanceSource.iInstanceIndex);

		if (nullptr != pInstanceObject)
		{
			if (pInstanceObject->Is_InActive())
				continue;

			// 에디터용 실객체가 존재하면 기존 상태 플래그로 해당 Instance만 렌더 캐시에서 제외한다.
			pInstanceObject->Set_InActive();
			m_TemporarilyHiddenInstances.push_back(pInstanceObject);
		}
		else
		{
			// 런타임 맵은 개별 실객체를 만들지 않으므로 공개된 렌더 캐시의 해당 인덱스만 일시적으로 제외한다.
			const CMapInstanceObject::RENDER_INSTANCE_CACHE* pRenderCache = RuntimeIter->second.pInstanceOwner->Get_RenderInstanceCache(Track.InstanceSource.iInstanceIndex);

			if (nullptr == pRenderCache || false == pRenderCache->bRenderable)
				continue;

			CMapInstanceObject::RENDER_INSTANCE_CACHE* pMutableRenderCache = const_cast<CMapInstanceObject::RENDER_INSTANCE_CACHE*>(pRenderCache);
			pMutableRenderCache->bRenderable = false;
			m_TemporarilyHiddenRuntimeInstances.emplace_back(RuntimeIter->second.pInstanceOwner, Track.InstanceSource.iInstanceIndex);
		}

		m_InstanceOverrideOwners.emplace(RuntimeIter->second.pInstanceOwner);
	}

	for (CMapInstanceObject* pOwner : m_InstanceOverrideOwners)
	{
		if (nullptr != pOwner)
			pOwner->Rebuild_RenderInstanceCache();
	}
}

void CCutScene_Manager::Post_MapUpdate()
{
	// 맵 컬링 제출이 끝난 같은 프레임에 원본 Instance 상태를 복원한다.
	Restore_InstanceOverrides();
}

void CCutScene_Manager::Stop()
{
	Restore_InstanceOverrides();
	Stop_CutSceneCamera();
	Stop_Sounds();
	Stop_Effects();
	Reset_ScreenOverlay();
	Reset_RuntimeObjects();
	Set_PlayState(CUTSCENE_PLAY_STATE::STOPPED);
	m_dPlaybackTime = 0.0;
	m_fCurrentTime = 0.f;
	m_iActiveCameraSectionID = UINT_MAX;
	m_bPlaybackCleanupPending = false;
	m_strActiveAssetName.clear();
	Restore_HUDLayer();
}

void CCutScene_Manager::Clear_Cache()
{
	// 캐시 Asset을 지우기 전에 그 Asset을 참조하는 사전 복제 객체부터 해제한다.
	Release_Chapter();
	m_CachedAssets.clear();
	m_LoadedEffectSourcePaths.clear();
}

HRESULT CCutScene_Manager::Prepare_Asset(const string& strAssetName, const CUTSCENE_ASSET& Asset)
{
	if (strAssetName.empty() || UINT_MAX == m_iObjectLevel || UINT_MAX == m_iModelLevel)
		return E_FAIL;

	if (m_PreparedAssets.end() != m_PreparedAssets.find(strAssetName))
		return S_OK;

	// 객체를 생성하기 전에 Runtime Player Track 중복을 검사해 불완전한 Prepared Asset을 남기지 않는다.
	_uint iRuntimePlayerTrackCount = {};

	for (const CUTSCENE_OBJECT_TRACK& Track : Asset.ObjectTracks)
	{
		if (CUTSCENE_OBJECT_TYPE::RUNTIME_PLAYER != Track.eObjectType)
			continue;

		++iRuntimePlayerTrackCount;

		if (iRuntimePlayerTrackCount > 1)
			return E_FAIL;
	}

	auto PreparedResult = m_PreparedAssets.emplace(strAssetName, CUTSCENE_PREPARED_ASSET{});
	CUTSCENE_PREPARED_ASSET& PreparedAsset = PreparedResult.first->second;
	vector<const CUTSCENE_OBJECT_TRACK*> OrderedTracks = {};
	OrderedTracks.reserve(Asset.ObjectTracks.size());

	for (const CUTSCENE_OBJECT_TRACK& Track : Asset.ObjectTracks)
		OrderedTracks.push_back(&Track);

	// 부모 액터를 먼저 Layer에 넣어 같은 프레임 Late_Update에서 자식이 최신 Bone 행렬을 받게 한다.
	stable_sort(OrderedTracks.begin(), OrderedTracks.end(), [&Asset](const CUTSCENE_OBJECT_TRACK* pLeft, const CUTSCENE_OBJECT_TRACK* pRight) { return CutScene_RuntimeGetObjectHierarchyDepth(Asset.ObjectTracks, *pLeft) < CutScene_RuntimeGetObjectHierarchyDepth(Asset.ObjectTracks, *pRight); });

	for (const CUTSCENE_OBJECT_TRACK* pTrack : OrderedTracks)
	{
		if (nullptr == pTrack)
			return E_FAIL;

		const CUTSCENE_OBJECT_TRACK& Track = *pTrack;

		if (UINT_MAX == Track.iObjectID || PreparedAsset.Objects.end() != PreparedAsset.Objects.find(Track.iObjectID))
			return E_FAIL;

		CUTSCENE_OBJECT_RUNTIME Runtime = {};
		const _float4x4 WorldMatrix = CutScene_RuntimeMakeWorldMatrix(Track.InitialTransform);

		if (CUTSCENE_OBJECT_TYPE::STATIC_PROP == Track.eObjectType)
		{
			CCutScene_Prop::CUTSCENE_PROP_DESC Desc = {};
			Desc.strMeshName = Track.strMeshName;
			Desc.strMaterialName = Track.strMaterialName;
			Desc.WorldMatrix = WorldMatrix;
			Desc.fEmissiveAmount = CutScene_RuntimeSampleFloat(Track.EmissiveKeys, 0.f, 0.f);
			Runtime.pProp = CCutScene_Prop::Create(m_pDevice, m_pContext, Desc);

			if (nullptr == Runtime.pProp || FAILED(m_pGameInstance->Add_GameObject(m_iObjectLevel, TEXT("Layer_CutScene"), Runtime.pProp)))
			{
				Safe_Release(Runtime.pProp);
				return E_FAIL;
			}

			// Layer와 매니저가 각각 하나의 참조를 소유해 챕터 해제 순서와 무관하게 안전하게 정리한다.
			Safe_AddRef(Runtime.pProp);
			Runtime.pProp->Set_CutSceneVisible(false);
		}
		else if (CUTSCENE_OBJECT_TYPE::ANIMATED_ACTOR == Track.eObjectType)
		{
			_wstring strModelPrototypeTag = Track.strModelPrototypeTag;
			if (strModelPrototypeTag.empty() && false == Track.strModelResourceName.empty())
				strModelPrototypeTag = L"Prototype_Component_Model_CutScene_" + Track.strModelResourceName;

			if (strModelPrototypeTag.empty())
				return E_FAIL;

			CCutScene_Actor::CUTSCENE_ACTOR_DESC Desc = {};
			// Static 모델을 Skinned로 생성하면 빈 Animator Pose를 Bone 수만큼 읽으므로 Track 타입을 그대로 사용한다.
			Desc.eVisualType = Track.bSkinnedModel ? CMapGameplayObject::VISUAL_TYPE::SKINNED : CMapGameplayObject::VISUAL_TYPE::STATIC;
			Desc.strModelPrototypeTag = strModelPrototypeTag;
			Desc.iModelLevel = Track.strModelResourceName.empty() ? Track.iModelLevel : m_iModelLevel;
			Desc.strAnimFolderTag = Track.strAnimFolderTag;
			Desc.strAnimTag = Track.strAnimTag;
			// 컷신 리소스는 Loader에서 TICKS 단위로 등록하므로 Animator 생성 단위도 동일하게 맞춘다.
			Desc.eAnimTimeUnit = ANIMATION_TIME_UNIT::TICKS;
			Desc.pRootBoneName = Track.strRootBoneName.c_str();
			Desc.bPlayDefault = false;
			Desc.bControllerEnabled = false;
			Desc.bApplyRootMotion = false;
			Desc.bUsePhysics = false;
			Desc.InitialTransform = Track.InitialTransform;

			Runtime.pActor = m_pGameInstance->Clone_Prototype<CCutScene_Actor>(m_iObjectLevel, TEXT("Prototype_GameObject_CutSceneActor"), &Desc);

			if (nullptr == Runtime.pActor || FAILED(m_pGameInstance->Add_GameObject(m_iObjectLevel, TEXT("Layer_CutScene"), Runtime.pActor)))
			{
				Safe_Release(Runtime.pActor);
				return E_FAIL;
			}

			Safe_AddRef(Runtime.pActor);
			Runtime.pActor->Reset_CutScene();
		}
		else if (CUTSCENE_OBJECT_TYPE::RUNTIME_PLAYER == Track.eObjectType)
		{
			// 실제 CPlayer는 GameInstance가 소유하므로 별도 컷신 객체를 Clone하지 않는다.
		}
		else
		{
			return E_FAIL;
		}

		if (CUTSCENE_OBJECT_TYPE::RUNTIME_PLAYER != Track.eObjectType && Track.InstanceSource.Is_Valid())
		{
			if (Track.InstanceSource.bStaticObject)
				Runtime.pStaticSource = Resolve_StaticSource(Track.InstanceSource);
			else
				Runtime.pInstanceOwner = Resolve_InstanceOwner(Track.InstanceSource);
		}

		PreparedAsset.Objects.emplace(Track.iObjectID, Runtime);
	}

	Bind_RuntimeAttachments(Asset, &PreparedAsset);
	return S_OK;
}

void CCutScene_Manager::Release_PreparedAssets()
{
	Restore_InstanceOverrides();

	for (pair<const string, CUTSCENE_PREPARED_ASSET>& PreparedPair : m_PreparedAssets)
	{
		for (pair<const _uint, CUTSCENE_OBJECT_RUNTIME>& RuntimePair : PreparedPair.second.Objects)
		{
			if (nullptr != RuntimePair.second.pActor)
			{
				RuntimePair.second.pActor->Reset_CutScene();
				RuntimePair.second.pActor->Clear_CutSceneAttachment();
			}

			if (nullptr != RuntimePair.second.pProp)
			{
				RuntimePair.second.pProp->Set_CutSceneVisible(false);
				RuntimePair.second.pProp->Clear_CutSceneAttachment();
			}
		}
	}

	if (nullptr != m_pGameInstance && UINT_MAX != m_iObjectLevel)
		m_pGameInstance->Clear_Layer(m_iObjectLevel, TEXT("Layer_CutScene"));

	for (pair<const string, CUTSCENE_PREPARED_ASSET>& PreparedPair : m_PreparedAssets)
	{
		for (pair<const _uint, CUTSCENE_OBJECT_RUNTIME>& RuntimePair : PreparedPair.second.Objects)
		{
			Safe_Release(RuntimePair.second.pActor);
			Safe_Release(RuntimePair.second.pProp);
			RuntimePair.second.pInstanceOwner = nullptr;
			RuntimePair.second.pStaticSource = nullptr;
		}
		PreparedPair.second.Objects.clear();
	}

	m_PreparedAssets.clear();
	m_iObjectLevel = UINT_MAX;
	m_iModelLevel = UINT_MAX;
}

void CCutScene_Manager::Bind_RuntimeAttachments(const CUTSCENE_ASSET& Asset, CUTSCENE_PREPARED_ASSET* pPreparedAsset)
{
	if (nullptr == pPreparedAsset)
		return;

	for (const CUTSCENE_OBJECT_TRACK& Track : Asset.ObjectTracks)
	{
		if (UINT_MAX == Track.iParentObjectID)
			continue;

		auto ChildIter = pPreparedAsset->Objects.find(Track.iObjectID);
		CCutScene_Actor* pParentActor = Find_Actor(pPreparedAsset, Track.iParentObjectID);

		if (pPreparedAsset->Objects.end() == ChildIter || nullptr == pParentActor)
			continue;

		if (nullptr != ChildIter->second.pActor)
			ChildIter->second.pActor->Set_CutSceneAttachment(pParentActor, Track.strAttachBoneName, Track.strChildAttachBoneName, 1.f, true, 1.f);

		if (nullptr != ChildIter->second.pProp)
			ChildIter->second.pProp->Set_CutSceneAttachment(pParentActor, Track.strAttachBoneName);
	}
}

HRESULT CCutScene_Manager::Load_Asset(const string& strAssetName, CUTSCENE_ASSET* pOutAsset)
{
	if (nullptr == pOutAsset || strAssetName.empty() || string::npos != strAssetName.find("..") || string::npos != strAssetName.find('/') || string::npos != strAssetName.find('\\'))
		return E_FAIL;

	static const string strExtension = ".CutScene.json";
	string strFileName = strAssetName;

	if (strFileName.size() < strExtension.size() || 0 != strFileName.compare(strFileName.size() - strExtension.size(), strExtension.size(), strExtension))
		strFileName += strExtension;

	const _wstring strFilePath = m_pGameInstance->Get_ResourcePath() + L"\\CutScene\\" + Engine::StringToWString(strFileName);
	string strErrorMessage = {};

	// JSON 경로 해석과 역직렬화는 Preload 단계에서 한 번만 수행한다.
	if (FAILED(CCutScene_Serializer::Load(strFilePath, pOutAsset, &strErrorMessage)))
	{
		OutputDebugStringA(("[CutScene Runtime Load Error] " + strErrorMessage + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CCutScene_Manager::Prepare_EffectResources(const CUTSCENE_ASSET& Asset)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	vector<string> EffectKeys = {};
	unordered_set<string> UniqueKeys = {};

	for (const CUTSCENE_EFFECT_EVENT& Event : Asset.EffectEvents)
	{
		if (false == Event.strEffectKey.empty() && UniqueKeys.emplace(Event.strEffectKey).second)
			EffectKeys.push_back(Event.strEffectKey);
	}

	for (const CUTSCENE_NOTIFY_BINDING& Binding : Asset.NotifyBindings)
	{
		if (CUTSCENE_NOTIFY_ACTION_TYPE::EFFECT == Binding.eActionType && false == Binding.strResourceKey.empty() && UniqueKeys.emplace(Binding.strResourceKey).second)
			EffectKeys.push_back(Binding.strResourceKey);
	}

	if (EffectKeys.empty())
		return S_OK;

	const _wstring strEffectsDirectory = m_pGameInstance->Get_ResourcePath() + L"\\Effects";
	vector<EFFECT_RESOURCE_CATALOG_ENTRY> CatalogEntries = {};
	string strErrorMessage = {};
	const HRESULT hrCatalog = CEffect_ResourceCatalog::Load(strEffectsDirectory, &CatalogEntries, &strErrorMessage);

	if (S_FALSE == hrCatalog)
	{
		OutputDebugStringA("[CutScene Runtime Effect Error] EffectLibrary.json is missing.\n");
		return E_FAIL;
	}

	if (FAILED(hrCatalog))
	{
		OutputDebugStringA(("[CutScene Runtime Effect Error] " + strErrorMessage + "\n").c_str());
		return E_FAIL;
	}

	for (const string& strEffectKey : EffectKeys)
	{
		const EFFECT_RESOURCE_CATALOG_ENTRY* pEntry = CEffect_ResourceCatalog::Find_ByKey(CatalogEntries, strEffectKey);

		if (nullptr == pEntry)
		{
			OutputDebugStringA(("[CutScene Runtime Effect Error] Effect key is not registered: " + strEffectKey + "\n").c_str());
			return E_FAIL;
		}

		const _wstring strSourcePath = CEffect_ResourceCatalog::Resolve_SourcePath(strEffectsDirectory, pEntry->strRelativeFilePath);

		if (strSourcePath.empty())
		{
			OutputDebugStringA(("[CutScene Runtime Effect Error] Invalid source path: " + strEffectKey + "\n").c_str());
			return E_FAIL;
		}

		auto LoadedIter = m_LoadedEffectSourcePaths.find(strEffectKey);
		if (m_LoadedEffectSourcePaths.end() != LoadedIter && LoadedIter->second == strSourcePath)
			continue;

		if (FAILED(m_pGameInstance->Load_Effect_File(strEffectKey, Engine::WStringToString(strSourcePath))))
		{
			OutputDebugStringA(("[CutScene Runtime Effect Error] Failed to load effect source: " + strEffectKey + "\n").c_str());
			return E_FAIL;
		}

		m_LoadedEffectSourcePaths[strEffectKey] = strSourcePath;
	}

	return S_OK;
}

HRESULT CCutScene_Manager::Ready_RuntimeCamera()
{
	CCamera::CAMERA_DESC CameraDesc = {};
	CameraDesc.vEye = _float3(0.f, 0.f, 0.f);
	CameraDesc.vAt = _float3(0.f, 0.f, 1.f);
	CameraDesc.fFovy = XMConvertToRadians(60.f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;

	CCutScene_Camera* pPrototype = CCutScene_Camera::Create(m_pDevice, m_pContext);

	if (nullptr == pPrototype)
		return E_FAIL;

	m_pCutSceneCamera = static_cast<CCutScene_Camera*>(pPrototype->Clone(&CameraDesc));
	Safe_Release(pPrototype);
	return nullptr == m_pCutSceneCamera ? E_FAIL : S_OK;
}

HRESULT CCutScene_Manager::Ready_ScreenOverlay()
{
	if (FAILED(m_pGameInstance->Open_UIObject("CutSceneScreen", L"GameCutScene_Screen", &m_hScreenOverlay)))
		return E_FAIL;

	CUIObject* pScreenObject = m_pGameInstance->Find_UIObject(m_hScreenOverlay);

	if (nullptr == pScreenObject)
		return E_FAIL;

	// UIObject가 수명을 소유하며 매니저는 재생 중 색만 갱신한다.
	m_pColorOverlay = pScreenObject->Find_Element(L"ColorOverlay");
	m_pFadeOverlay = pScreenObject->Find_Element(L"FadeOverlay");

	if (nullptr == m_pColorOverlay || nullptr == m_pFadeOverlay)
		return E_FAIL;

	Reset_ScreenOverlay();
	return S_OK;
}

void CCutScene_Manager::Sort_CutScene()
{
	for (CUTSCENE_CAMERA_TRACK& Track : m_Asset.CameraTracks)
		stable_sort(Track.Keys.begin(), Track.Keys.end(), [](const CUTSCENE_CAMERA_KEY& Left, const CUTSCENE_CAMERA_KEY& Right) { return Left.fTime < Right.fTime; });

	stable_sort(m_Asset.ObjectTracks.begin(), m_Asset.ObjectTracks.end(), [this](const CUTSCENE_OBJECT_TRACK& Left, const CUTSCENE_OBJECT_TRACK& Right) { return CutScene_RuntimeGetObjectHierarchyDepth(m_Asset.ObjectTracks, Left) < CutScene_RuntimeGetObjectHierarchyDepth(m_Asset.ObjectTracks, Right); });

	for (CUTSCENE_OBJECT_TRACK& Track : m_Asset.ObjectTracks)
	{
		stable_sort(Track.TransformKeys.begin(), Track.TransformKeys.end(), [](const CUTSCENE_TRANSFORM_KEY& Left, const CUTSCENE_TRANSFORM_KEY& Right) { return Left.fTime < Right.fTime; });
		stable_sort(Track.EmissiveKeys.begin(), Track.EmissiveKeys.end(), [](const CUTSCENE_FLOAT_KEY& Left, const CUTSCENE_FLOAT_KEY& Right) { return Left.fTime < Right.fTime; });
		stable_sort(Track.DissolveKeys.begin(), Track.DissolveKeys.end(), [](const CUTSCENE_DISSOLVE_KEY& Left, const CUTSCENE_DISSOLVE_KEY& Right) { return Left.fTime < Right.fTime; });
		stable_sort(Track.VisibilityKeys.begin(), Track.VisibilityKeys.end(), [](const CUTSCENE_VISIBILITY_KEY& Left, const CUTSCENE_VISIBILITY_KEY& Right) { return Left.fTime < Right.fTime; });
		stable_sort(Track.AttachmentKeys.begin(), Track.AttachmentKeys.end(), [](const CUTSCENE_ATTACHMENT_KEY& Left, const CUTSCENE_ATTACHMENT_KEY& Right) { return Left.fTime < Right.fTime; });
		stable_sort(Track.AnimationSections.begin(), Track.AnimationSections.end(), [](const CUTSCENE_ANIMATION_SECTION& Left, const CUTSCENE_ANIMATION_SECTION& Right) { return Left.fStartTime < Right.fStartTime; });
	}

	stable_sort(m_Asset.CameraSceneSections.begin(), m_Asset.CameraSceneSections.end(), [](const CUTSCENE_CAMERA_SCENE_SECTION& Left, const CUTSCENE_CAMERA_SCENE_SECTION& Right) { return Left.fStartTime < Right.fStartTime; });
	stable_sort(m_Asset.ScreenKeys.begin(), m_Asset.ScreenKeys.end(), [](const CUTSCENE_SCREEN_KEY& Left, const CUTSCENE_SCREEN_KEY& Right) { return Left.fTime < Right.fTime; });
	stable_sort(m_Asset.SoundEvents.begin(), m_Asset.SoundEvents.end(), [](const CUTSCENE_SOUND_EVENT& Left, const CUTSCENE_SOUND_EVENT& Right) { return Left.fStartTime < Right.fStartTime; });
}

void CCutScene_Manager::Evaluate_Camera(_float fTime)
{
	const CUTSCENE_CAMERA_SCENE_SECTION* pActiveSection = nullptr;
	const _uint iPreviousSectionID = m_iActiveCameraSectionID;

	for (const CUTSCENE_CAMERA_SCENE_SECTION& Section : m_Asset.CameraSceneSections)
	{
		const _bool bFinalFrame = fTime >= m_Asset.fDuration && Section.fEndTime >= m_Asset.fDuration;

		if (fTime >= Section.fStartTime && (fTime < Section.fEndTime || bFinalFrame))
		{
			pActiveSection = &Section;
			break;
		}
	}

	if (nullptr == pActiveSection)
	{
		Stop_CutSceneCamera();
		return;
	}

	const CUTSCENE_CAMERA_TRACK* pTrack = nullptr;

	for (const CUTSCENE_CAMERA_TRACK& Track : m_Asset.CameraTracks)
	{
		if (Track.iTrackID == pActiveSection->iCameraTrackID)
		{
			pTrack = &Track;
			break;
		}
	}

	if (nullptr == pTrack || nullptr == m_pCutSceneCamera)
	{
		Stop_CutSceneCamera();
		return;
	}

	const _float fTrackTime = max(0.f, (fTime - pActiveSection->fStartTime) * pActiveSection->fPlayRate);
	CUTSCENE_CAMERA_KEY EvaluatedKey = {};

	if (false == CutScene_RuntimeSampleCamera(*pTrack, fTrackTime, &EvaluatedKey))
	{
		Stop_CutSceneCamera();
		return;
	}

	m_pCutSceneCamera->Set_CameraPose(XMLoadFloat3(&EvaluatedKey.vPosition), XMLoadFloat4(&EvaluatedKey.vQuaternion), EvaluatedKey.fVerticalFovy);
	const _bool bCutSceneCameraPlaying = m_pGameInstance->Is_CutSceneCameraPlaying();

	if (false == bCutSceneCameraPlaying)
	{
		if (FAILED(m_pGameInstance->Begin_CutSceneCamera(m_pCutSceneCamera, pActiveSection->fBlendIn)))
			return;
	}
	else if (UINT_MAX != iPreviousSectionID && iPreviousSectionID != pActiveSection->iSectionID)
	{
		_float fPreviousBlendOut = {};

		for (const CUTSCENE_CAMERA_SCENE_SECTION& Section : m_Asset.CameraSceneSections)
		{
			if (Section.iSectionID == iPreviousSectionID)
			{
				fPreviousBlendOut = Section.fBlendOut;
				break;
			}
		}

		// Camera Cut 경계에서는 이전 BlendOut과 새 BlendIn 중 긴 시간을 사용한다.
		m_pGameInstance->Transition_CutSceneCamera(max(fPreviousBlendOut, pActiveSection->fBlendIn));
	}

	m_iActiveCameraSectionID = pActiveSection->iSectionID;
}

void CCutScene_Manager::Evaluate_Objects(_float fTime, _bool bSeek)
{
	CUTSCENE_PREPARED_ASSET* pPreparedAsset = Find_ActivePreparedAsset();
	if (nullptr == pPreparedAsset)
		return;

	for (const CUTSCENE_OBJECT_TRACK& Track : m_Asset.ObjectTracks)
	{
		auto RuntimeIter = pPreparedAsset->Objects.find(Track.iObjectID);
		if (pPreparedAsset->Objects.end() == RuntimeIter)
			continue;

		if (CUTSCENE_OBJECT_TYPE::RUNTIME_PLAYER == Track.eObjectType)
		{
			Evaluate_RuntimePlayer(Track, fTime, bSeek);
			continue;
		}

		const _bool bVisible = fTime >= Track.fSpawnTime && fTime < Track.fDespawnTime && CutScene_RuntimeSampleVisibility(Track.VisibilityKeys, true, fTime);
		const CUTSCENE_ATTACHMENT_KEY AttachmentKey = CutScene_RuntimeSampleAttachment(Track, fTime);
		CCutScene_Actor* pAttachmentParent = AttachmentKey.bAttached ? Find_Actor(pPreparedAsset, AttachmentKey.iParentObjectID) : nullptr;
		const _bool bAttached = nullptr != pAttachmentParent;
		const _float fAttachmentBlendWeight = CutScene_RuntimeGetAttachmentBlendWeight(AttachmentKey, fTime, bSeek);
		const _float fParentRotationWeight = CutScene_RuntimeGetParentRotationWeight(AttachmentKey, fTime);
		const CUTSCENE_TRANSFORM_VALUE Transform = CutScene_RuntimeSampleTransform(Track.TransformKeys, Track.InitialTransform, fTime);
		const _float fEmissive = CutScene_RuntimeSampleFloat(Track.EmissiveKeys, 0.f, fTime);
		const CUTSCENE_DISSOLVE_KEY Dissolve = CutScene_RuntimeSampleDissolve(Track.DissolveKeys, fTime);

		if (nullptr != RuntimeIter->second.pProp)
		{
			RuntimeIter->second.pProp->Set_CutSceneVisible(bVisible);
			RuntimeIter->second.pProp->Set_EmissiveAmount(fEmissive);
			RuntimeIter->second.pProp->Set_CutSceneDissolve(Dissolve.fAmount, Dissolve.vColor);

			if (bAttached)
				RuntimeIter->second.pProp->Set_CutSceneAttachment(pAttachmentParent, AttachmentKey.strAttachBoneName, fAttachmentBlendWeight);

			RuntimeIter->second.pProp->Set_CutSceneAttachmentActive(bAttached);
			const _float4x4 WorldMatrix = CutScene_RuntimeMakeWorldMatrix(Transform);
			RuntimeIter->second.pProp->Set_CutSceneTransform(XMLoadFloat4x4(&WorldMatrix));
		}

		if (nullptr != RuntimeIter->second.pActor)
		{
			RuntimeIter->second.pActor->Set_CutSceneVisible(bVisible);
			RuntimeIter->second.pActor->Set_EmissiveAmount(fEmissive);
			RuntimeIter->second.pActor->Set_CutSceneDissolve(Dissolve.fAmount, Dissolve.vColor);

			// 키가 지정된 Mesh는 매 평가마다 true부터 다시 계산해 역방향 Seek에서도 즉시 복구한다.
			for (const CUTSCENE_MESH_VISIBILITY_KEY& TargetKey : Track.MeshVisibilityKeys)
			{
				_bool bMeshVisible = true;

				for (const CUTSCENE_MESH_VISIBILITY_KEY& Key : Track.MeshVisibilityKeys)
					if (Key.iMeshIndex == TargetKey.iMeshIndex && Key.fTime <= fTime)
						bMeshVisible = Key.bVisible;

				RuntimeIter->second.pActor->Set_MeshVisible(TargetKey.iMeshIndex, bMeshVisible);
			}

			if (bAttached)
				RuntimeIter->second.pActor->Set_CutSceneAttachment(pAttachmentParent, AttachmentKey.strAttachBoneName, AttachmentKey.strChildAttachBoneName, fAttachmentBlendWeight, AttachmentKey.bInheritParentRotation, fParentRotationWeight);

			RuntimeIter->second.pActor->Set_CutSceneAttachmentActive(bAttached);
			// 렌더링 숨김은 타임라인 Seek가 아니다. 실제 Seek 요청만 애니메이션 재구축에 전달한다.
			RuntimeIter->second.pActor->Queue_Evaluation(Transform, Track.AnimationSections, fTime, bSeek);
		}
	}
}

void CCutScene_Manager::Evaluate_RuntimePlayer(const CUTSCENE_OBJECT_TRACK& Track, _float fTime, _bool bSeek)
{
	if (nullptr == m_pGameInstance)
		return;

	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

	if (nullptr == pPlayer)
		return;

	const _bool bActive = fTime >= Track.fSpawnTime && fTime < Track.fDespawnTime;

	if (false == bActive)
	{
		pPlayer->Clear_CutSceneControl();
		return;
	}

	// 기존 Object Track과 동일하게 위치 키는 다음 키까지 유지하고 키 시점에 즉시 적용한다.
	const CUTSCENE_TRANSFORM_VALUE Transform = CutScene_RuntimeSampleTransform(Track.TransformKeys, Track.InitialTransform, fTime);
	const CUTSCENE_ANIMATION_SECTION* pAnimationSection = CutScene_RuntimeFindAnimationSection(Track.AnimationSections, fTime);

	if (nullptr == pAnimationSection || false == pAnimationSection->bUseRootMotion)
	{
		pPlayer->Apply_CutSceneControl(Transform, pAnimationSection, fTime);
		return;
	}

	CAnimDriver* pAnimDriver = pPlayer->Get_AnimDriverComponent();
	CCT* pCCT = pPlayer->Get_CCTComponent();
	CTransform* pTransform = pPlayer->Get_Transform();

	if (nullptr == pAnimDriver || nullptr == pCCT || nullptr == pTransform)
		return;

	const _bool bBeginControl = false == pAnimDriver->Is_CutSceneControlled();
	const _bool bSectionChanged = pAnimDriver->Get_CutSceneSectionID() != pAnimationSection->iSectionID;

	if (bBeginControl)
	{
		// 기존 Player 진입 정리 경로는 그대로 사용하고 Root Delta 소비만 Manager가 담당한다.
		pPlayer->Apply_CutSceneControl(Transform, pAnimationSection, fTime);
	}
	else
	{
		if (bSeek || (bSectionChanged && pAnimationSection->bResetRootMotion))
		{
			Respawn_Point CutScenePoint = {};
			CutScenePoint.vPosition = _float4(Transform.vPosition.x, Transform.vPosition.y, Transform.vPosition.z, 1.f);
			CutScenePoint.fYaw = XMConvertToRadians(Transform.vRotationDegree.y);
			pPlayer->Teleport(CutScenePoint);
		}

		pAnimDriver->Apply_CutSceneAnimation(pAnimationSection, fTime, bSeek);
	}

	_float3 vRootTranslation = {};
	_float4 vRootRotation = { 0.f, 0.f, 0.f, 1.f };

	if (false == pAnimDriver->Get_CutSceneRootMotionDelta(&vRootTranslation, &vRootRotation))
		return;

	// Animator local Root Delta를 현재 Player 방향으로 변환해 CCT에 한 번만 적용한다.
	const _vector vCurrentQuat = pTransform->Get_Quat();
	const _vector vWorldDelta = XMVector3Rotate(XMLoadFloat3(&vRootTranslation), vCurrentQuat);
	// CCT의 경과 시간도 현재 컷신 FPS를 사용해 30/60 FPS Asset 모두 동일하게 처리한다.
	const _float fCCTTimeDelta = 1.f / max(m_Asset.fFramesPerSecond, 1.f);
	const CCTResult Result = pCCT->Move(vWorldDelta, fCCTTimeDelta);
	pTransform->Set_State(STATE::POSITION, XMLoadFloat4(&Result.vSimulatedPosition));

	_vector vRootAxis = XMVectorZero();
	_float fRootAngle = {};
	XMQuaternionToAxisAngle(&vRootAxis, &fRootAngle, XMQuaternionNormalize(XMLoadFloat4(&vRootRotation)));
	const _float fRootYaw = fRootAngle * XMVectorGetY(vRootAxis);

	if (fabsf(fRootYaw) > FLT_EPSILON)
		pTransform->Set_Quat(XMQuaternionNormalize(XMQuaternionRotationMatrix(XMMatrixRotationY(fRootYaw) * XMMatrixRotationQuaternion(vCurrentQuat))));
}

void CCutScene_Manager::Evaluate_Screen(_float fTime)
{
	if (nullptr == m_pColorOverlay || nullptr == m_pFadeOverlay)
		return;

	if (m_Asset.ScreenKeys.empty())
	{
		m_pColorOverlay->Set_Color(_float4(0.f, 0.f, 0.f, 0.f));
		m_pFadeOverlay->Set_Color(_float4(0.f, 0.f, 0.f, 0.f));
		return;
	}

	const CUTSCENE_SCREEN_KEY* pLeft = &m_Asset.ScreenKeys.front();
	const CUTSCENE_SCREEN_KEY* pRight = pLeft;

	if (fTime >= m_Asset.ScreenKeys.back().fTime)
	{
		pLeft = &m_Asset.ScreenKeys.back();
		pRight = pLeft;
	}
	else
	{
		for (_uint i = 1; i < static_cast<_uint>(m_Asset.ScreenKeys.size()); ++i)
		{
			if (fTime <= m_Asset.ScreenKeys[i].fTime)
			{
				// 현재 시간이 속한 구간의 양쪽 키를 선택해 에디터와 같은 기준으로 보간한다.
				pLeft = &m_Asset.ScreenKeys[i - 1];
				pRight = &m_Asset.ScreenKeys[i];
				break;
			}
		}
	}

	const _float fRatio = CutScene_RuntimeKeyRatio(fTime, pLeft->fTime, pRight->fTime);
	_float4 vScreenColor = {};
	XMStoreFloat4(&vScreenColor, XMVectorLerp(XMLoadFloat4(&pLeft->vColor), XMLoadFloat4(&pRight->vColor), fRatio));
	const _float fColorAmount = CutScene_RuntimeClamp01(pLeft->fColorAmount + (pRight->fColorAmount - pLeft->fColorAmount) * fRatio);
	const _float fFadeAmount = CutScene_RuntimeClamp01(pLeft->fFadeAmount + (pRight->fFadeAmount - pLeft->fFadeAmount) * fRatio);

	// 에디터와 동일하게 색 오버레이를 먼저, 검은 페이드를 그 위에 출력한다.
	m_pColorOverlay->Set_Color(_float4(vScreenColor.x, vScreenColor.y, vScreenColor.z, CutScene_RuntimeClamp01(vScreenColor.w * fColorAmount)));
	m_pFadeOverlay->Set_Color(_float4(0.f, 0.f, 0.f, fFadeAmount));
}

void CCutScene_Manager::Dispatch_Sounds(_float fPreviousTime, _float fCurrentTime)
{
	for (const CUTSCENE_SOUND_EVENT& Event : m_Asset.SoundEvents)
	{
		if (fPreviousTime < Event.fStartTime && fCurrentTime >= Event.fStartTime)
			Start_SoundEvent(Event);

		if (Event.fEndTime > Event.fStartTime && fPreviousTime < Event.fEndTime && fCurrentTime >= Event.fEndTime)
			Stop_SoundEvent(Event);
	}
}

void CCutScene_Manager::Start_SoundEvent(const CUTSCENE_SOUND_EVENT& Event)
{
	if (m_SoundEventHandles.end() != m_SoundEventHandles.find(Event.iEventID))
		return;

	_uint iSoundHandle = UINT_MAX;

	if (Event.bBGM)
	{
		// 컷신 BGM은 기존 BGM을 교체하고 컷신 종료 또는 EndTime에서 정리한다.
		m_pGameInstance->Play_BGM(Event.strSoundKey, Event.fVolume);
	}
	else if (Event.bUseWorldPosition || UINT_MAX != Event.iObjectID)
	{
		_float3 vPosition = Event.vWorldPosition;
		const _float4x4* pObjectWorld = Find_ObjectWorld(Event.iObjectID);

		if (nullptr != pObjectWorld)
			vPosition = _float3(pObjectWorld->_41, pObjectWorld->_42, pObjectWorld->_43);

		iSoundHandle = m_pGameInstance->Play_SoundAt(Event.strSoundKey, Event.eChannel, vPosition, Event.fVolume, Event.bLoop);
	}
	else
	{
		// 2D 사운드는 키 추적을 사용해 Event 종료 시 해당 사운드만 정지한다.
		m_pGameInstance->Play_Sound(Event.strSoundKey, Event.eChannel, Event.fVolume, Event.bLoop, true);
	}

	m_SoundEventHandles.emplace(Event.iEventID, iSoundHandle);
}

void CCutScene_Manager::Stop_SoundEvent(const CUTSCENE_SOUND_EVENT& Event)
{
	const auto RuntimeIter = m_SoundEventHandles.find(Event.iEventID);

	if (m_SoundEventHandles.end() == RuntimeIter)
		return;

	if (UINT_MAX != RuntimeIter->second)
		m_pGameInstance->Stop_SoundSource(RuntimeIter->second);
	else if (Event.bBGM)
		m_pGameInstance->Stop_BGM();
	else
		m_pGameInstance->StopSound_ByKey(Event.strSoundKey);

	m_SoundEventHandles.erase(RuntimeIter);
}

void CCutScene_Manager::Stop_Sounds()
{
	for (const CUTSCENE_SOUND_EVENT& Event : m_Asset.SoundEvents)
		Stop_SoundEvent(Event);

	m_SoundEventHandles.clear();
}

void CCutScene_Manager::Dispatch_Effects(_float fPreviousTime, _float fCurrentTime)
{
	for (const CUTSCENE_EFFECT_EVENT& Event : m_Asset.EffectEvents)
	{
		if (fPreviousTime < Event.fStartTime && fCurrentTime >= Event.fStartTime)
			Start_EffectEvent(Event);

		if (Event.fEndTime > Event.fStartTime && fPreviousTime < Event.fEndTime && fCurrentTime >= Event.fEndTime)
			Stop_EffectEvent(Event);
	}
}

void CCutScene_Manager::Start_EffectEvent(const CUTSCENE_EFFECT_EVENT& Event)
{
	auto RuntimeIter = m_EffectEventHandles.find(Event.iEventID);
	if (m_EffectEventHandles.end() != RuntimeIter && nullptr != RuntimeIter->second && false == RuntimeIter->second->Is_Finished())
		return;

	if (m_EffectEventHandles.end() != RuntimeIter)
	{
		Safe_Release(RuntimeIter->second);
		m_EffectEventHandles.erase(RuntimeIter);
	}

	CUTSCENE_PREPARED_ASSET* pPreparedAsset = Find_ActivePreparedAsset();
	CCutScene_Actor* pActor = Find_Actor(pPreparedAsset, Event.iObjectID);
	const _float4x4* pObjectWorld = Find_ObjectWorld(Event.iObjectID);
	CEffect* pEffect = nullptr;

	switch (Event.eAttachType)
	{
	case CUTSCENE_EFFECT_ATTACH_TYPE::WORLD:
		pEffect = m_pGameInstance->Play_Effect(Event.strEffectKey, _float3(Event.vWorldPosition.x + Event.vOffset.x, Event.vWorldPosition.y + Event.vOffset.y, Event.vWorldPosition.z + Event.vOffset.z));
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::OBJECT:
		if (nullptr != pObjectWorld)
			pEffect = m_pGameInstance->Play_Effect(Event.strEffectKey, nullptr, pObjectWorld, Event.vOffset, Event.vAngle);
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::BONE:
		if (nullptr != pActor && false == Event.strBoneName.empty())
			pEffect = m_pGameInstance->Play_Effect(Event.strEffectKey, pActor->Get_Model(), Event.strBoneName.c_str(), pActor->Get_WorldTransform(), Event.vOffset, Event.vAngle);
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::BONE_PAIR:
		if (nullptr != pActor && false == Event.strBoneName.empty() && false == Event.strSecondaryBoneName.empty())
			pEffect = m_pGameInstance->Play_Effect(Event.strEffectKey, pActor->Get_Model(), Event.strBoneName.c_str(), Event.strSecondaryBoneName.c_str(), pActor->Get_WorldTransform(), Event.vOffset, Event.vAngle);
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::MODEL:
		if (nullptr != pActor)
			pEffect = m_pGameInstance->Play_Effect_Model(Event.strEffectKey, pActor->Get_Model(), pActor->Get_WorldTransform(), Event.vOffset, Event.vAngle);
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::PARTS:
		{
			vector<CModel*> Parts = {};
			for (const _uint iPartObjectID : Event.PartObjectIDs)
			{
				CCutScene_Actor* pPartActor = Find_Actor(pPreparedAsset, iPartObjectID);
				if (nullptr != pPartActor && nullptr != pPartActor->Get_Model())
					Parts.push_back(pPartActor->Get_Model());
			}

			if (false == Parts.empty() && nullptr != pObjectWorld)
				pEffect = m_pGameInstance->Play_Effect_Parts(Event.strEffectKey, Parts, pObjectWorld, Event.vOffset, Event.vAngle);
		}
		break;
	case CUTSCENE_EFFECT_ATTACH_TYPE::OBJECT_PAIR:
		{
			const _float4x4* pSecondaryObjectWorld = Find_ObjectWorld(Event.iSecondaryObjectID);
			if (nullptr != pObjectWorld && nullptr != pSecondaryObjectWorld)
				pEffect = m_pGameInstance->Play_Effect(Event.strEffectKey, pObjectWorld, pSecondaryObjectWorld, nullptr, Event.vOffset, Event.vAngle);
		}
		break;
	default:
		break;
	}

	if (nullptr != pEffect)
	{
		Safe_AddRef(pEffect);
		m_EffectEventHandles.emplace(Event.iEventID, pEffect);
	}
}

void CCutScene_Manager::Stop_EffectEvent(const CUTSCENE_EFFECT_EVENT& Event)
{
	auto RuntimeIter = m_EffectEventHandles.find(Event.iEventID);
	if (m_EffectEventHandles.end() == RuntimeIter)
		return;

	if (nullptr != RuntimeIter->second)
	{
		// 컷신 시간 경계를 넘긴 루프 이펙트는 잔여 파티클 없이 즉시 종료한다.
		RuntimeIter->second->Stop_Immediate();
		Safe_Release(RuntimeIter->second);
	}

	m_EffectEventHandles.erase(RuntimeIter);
}

void CCutScene_Manager::Stop_Effects()
{
	for (auto& Pair : m_EffectEventHandles)
	{
		if (nullptr != Pair.second)
		{
			Pair.second->Stop_Immediate();
			Safe_Release(Pair.second);
		}
	}

	m_EffectEventHandles.clear();
}

void CCutScene_Manager::Reset_RuntimeObjects()
{
	Reset_RuntimePlayer();

	CUTSCENE_PREPARED_ASSET* pPreparedAsset = Find_ActivePreparedAsset();
	if (nullptr == pPreparedAsset)
		return;

	for (pair<const _uint, CUTSCENE_OBJECT_RUNTIME>& RuntimePair : pPreparedAsset->Objects)
	{
		if (nullptr != RuntimePair.second.pActor)
			RuntimePair.second.pActor->Reset_CutScene();

		if (nullptr != RuntimePair.second.pProp)
		{
			RuntimePair.second.pProp->Set_CutSceneVisible(false);
			RuntimePair.second.pProp->Set_CutSceneAttachmentActive(false);
		}
	}
}

void CCutScene_Manager::Reset_RuntimePlayer()
{
	if (nullptr == m_pGameInstance)
		return;

	CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());

	if (nullptr != pPlayer)
		pPlayer->Clear_CutSceneControl();
}

void CCutScene_Manager::Restore_InstanceOverrides()
{
	for (CMapObject* pInstanceObject : m_TemporarilyHiddenInstances)
	{
		if (nullptr != pInstanceObject)
			pInstanceObject->Set_Active();
	}

	for (const pair<CMapInstanceObject*, _uint>& RuntimeInstance : m_TemporarilyHiddenRuntimeInstances)
	{
		if (nullptr == RuntimeInstance.first)
			continue;

		const CMapInstanceObject::RENDER_INSTANCE_CACHE* pRenderCache = RuntimeInstance.first->Get_RenderInstanceCache(RuntimeInstance.second);

		if (nullptr == pRenderCache)
			continue;

		// Pre_MapUpdate에서 제외한 런타임 Instance만 원래 렌더 가능 상태로 되돌린다.
		CMapInstanceObject::RENDER_INSTANCE_CACHE* pMutableRenderCache = const_cast<CMapInstanceObject::RENDER_INSTANCE_CACHE*>(pRenderCache);
		pMutableRenderCache->bRenderable = true;
	}

	for (CMapInstanceObject* pOwner : m_InstanceOverrideOwners)
	{
		if (nullptr != pOwner)
			pOwner->Rebuild_RenderInstanceCache();
	}

	m_TemporarilyHiddenInstances.clear();
	m_TemporarilyHiddenRuntimeInstances.clear();
	m_InstanceOverrideOwners.clear();
}

void CCutScene_Manager::Stop_CutSceneCamera()
{
	if (nullptr != m_pGameInstance && m_pGameInstance->Is_CutSceneCameraPlaying())
	{
		_float fBlendOut = {};

		for (const CUTSCENE_CAMERA_SCENE_SECTION& Section : m_Asset.CameraSceneSections)
		{
			if (Section.iSectionID == m_iActiveCameraSectionID)
			{
				fBlendOut = Section.fBlendOut;
				break;
			}
		}

		// CameraManager가 저장한 원래 메인 카메라 모드와 Pose로 복귀한다.
		m_pGameInstance->End_CutSceneCamera(fBlendOut);
	}

	m_iActiveCameraSectionID = UINT_MAX;
}

void CCutScene_Manager::Reset_ScreenOverlay()
{
	if (nullptr != m_pColorOverlay)
		m_pColorOverlay->Set_Color(_float4(0.f, 0.f, 0.f, 0.f));

	if (nullptr != m_pFadeOverlay)
		m_pFadeOverlay->Set_Color(_float4(0.f, 0.f, 0.f, 0.f));

	if (nullptr != m_pGameInstance && m_hScreenOverlay.Is_Valid())
		m_pGameInstance->Close_UIObject(m_hScreenOverlay);
}

void CCutScene_Manager::Finish_Playback()
{
	// Stop 요청과 구분하기 위해 정상 재생이 끝난 Asset 이름을 정리 전에 보관한다.
	const string strFinishedAssetName = m_strActiveAssetName;
	_bool bKeepCutSceneCamera = { false };

	Restore_InstanceOverrides();
	Stop_Sounds();
	Stop_Effects();
	Reset_ScreenOverlay();
	Reset_RuntimeObjects();
	Set_PlayState(CUTSCENE_PLAY_STATE::STOPPED);
	m_bPlaybackCleanupPending = false;
	m_dPlaybackTime = 0.0;
	m_fCurrentTime = 0.f;
	m_iActiveCameraSectionID = UINT_MAX;
	m_strActiveAssetName.clear();

	// 런타임 객체를 정리한 뒤 Level에 정상 완료를 통지한다.
	// Chapter 전환을 시작한 경우에는 전환 FadeOut이 끝날 때까지 마지막 컷신 카메라를 유지한다.
	if (false == strFinishedAssetName.empty() && static_cast<_bool>(m_PlaybackFinishedCallback))
		bKeepCutSceneCamera = m_PlaybackFinishedCallback(strFinishedAssetName);

	if (false == bKeepCutSceneCamera)
		Stop_CutSceneCamera();

	Restore_HUDLayer();
}

CUTSCENE_PREPARED_ASSET* CCutScene_Manager::Find_ActivePreparedAsset()
{
	auto Iter = m_PreparedAssets.find(m_strActiveAssetName);
	return m_PreparedAssets.end() == Iter ? nullptr : &Iter->second;
}

CCutScene_Actor* CCutScene_Manager::Find_Actor(CUTSCENE_PREPARED_ASSET* pPreparedAsset, _uint iObjectID) const
{
	if (nullptr == pPreparedAsset)
		return nullptr;

	auto Iter = pPreparedAsset->Objects.find(iObjectID);
	return pPreparedAsset->Objects.end() == Iter ? nullptr : Iter->second.pActor;
}

const _float4x4* CCutScene_Manager::Find_ObjectWorld(_uint iObjectID) const
{
	for (const CUTSCENE_OBJECT_TRACK& Track : m_Asset.ObjectTracks)
	{
		if (Track.iObjectID != iObjectID || CUTSCENE_OBJECT_TYPE::RUNTIME_PLAYER != Track.eObjectType)
			continue;

		if (nullptr == m_pGameInstance)
			return nullptr;

		CPlayer* pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_Player());
		return nullptr == pPlayer ? nullptr : pPlayer->Get_WorldTransform();
	}

	const auto PreparedAssetIter = m_PreparedAssets.find(m_strActiveAssetName);

	if (m_PreparedAssets.end() == PreparedAssetIter)
		return nullptr;

	const auto ObjectIter = PreparedAssetIter->second.Objects.find(iObjectID);

	if (PreparedAssetIter->second.Objects.end() == ObjectIter)
		return nullptr;

	if (nullptr != ObjectIter->second.pActor)
		return ObjectIter->second.pActor->Get_WorldTransform();

	if (nullptr != ObjectIter->second.pProp)
		return ObjectIter->second.pProp->Get_WorldTransform();

	return nullptr;
}

CMapInstanceObject* CCutScene_Manager::Resolve_InstanceOwner(const CUTSCENE_INSTANCE_SOURCE& Source) const
{
	if (false == Source.Is_Valid() || Source.bStaticObject)
		return nullptr;

	CMap* pMap = m_pGameInstance->Get_Map(Source.strMapName);
	if (nullptr == pMap)
		return nullptr;

	_uint iInstanceGroupIndex = {};
	for (CMapObject* pMapObject : pMap->Get_Objects())
	{
		CMapInstanceObject* pInstanceOwner = dynamic_cast<CMapInstanceObject*>(pMapObject);
		if (nullptr == pInstanceOwner)
			continue;

		if (iInstanceGroupIndex == Source.iGroupIndex)
			return Source.iInstanceIndex < pInstanceOwner->Get_InstanceCount() ? pInstanceOwner : nullptr;

		++iInstanceGroupIndex;
	}

	return nullptr;
}

CMapObject* CCutScene_Manager::Resolve_StaticSource(const CUTSCENE_INSTANCE_SOURCE& Source) const
{
	if (false == Source.Is_Valid() || false == Source.bStaticObject)
		return nullptr;

	CMap* pMap = m_pGameInstance->Get_Map(Source.strMapName);
	if (nullptr == pMap)
		return nullptr;

	_uint iStaticGroupIndex = {};
	for (CMapObject* pMapObject : pMap->Get_Objects())
	{
		if (nullptr != dynamic_cast<CMapInstanceObject*>(pMapObject))
			continue;

		if (iStaticGroupIndex == Source.iGroupIndex)
			return pMapObject;

		++iStaticGroupIndex;
	}

	return nullptr;
}

CCutScene_Manager* CCutScene_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const function<_bool()>& CanPlayCallback, const function<void(_bool)>& PlaybackStateCallback, const function<_bool(const string&)>& PlaybackFinishedCallback)
{
	CCutScene_Manager* pInstance = new CCutScene_Manager(pDevice, pContext);

	if (FAILED(pInstance->Initialize(CanPlayCallback, PlaybackStateCallback, PlaybackFinishedCallback)))
	{
		MSG_BOX("Failed to Create : CCutScene_Manager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCutScene_Manager::Free()
{
	if (nullptr != m_pGameInstance)
		m_pGameInstance->UnSubscribe_AllEvent(this);

	Clear_Cache();
	Restore_HUDLayer();
	m_CanPlayCallback = nullptr;
	m_PlaybackStateCallback = nullptr;
	m_PlaybackFinishedCallback = nullptr;
	Safe_Release(m_pCutSceneCamera);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
	__super::Free();
}

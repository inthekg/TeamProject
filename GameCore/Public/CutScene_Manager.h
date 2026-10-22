#pragma once

#include "Base.h"
#include "CutScene_Data.h"
#include "IListener.h"
#include "UI_RuntimeData.h"

NS_BEGIN(Engine)
class CEffect;
class CGameInstance;
class CUIElement;
class CMapInstanceObject;
class CMapObject;
NS_END

NS_BEGIN(GameCore)

class CCutScene_Camera;
class CCutScene_Actor;
class CCutScene_Prop;

// 매니저가 챕터 진입 때 미리 만들어 두는 오브젝트 한 개의 런타임 참조다.
typedef struct tagCutSceneObjectRuntime
{
	CCutScene_Actor*				pActor = { nullptr };
	CCutScene_Prop*				pProp = { nullptr };
	Engine::CMapInstanceObject*	pInstanceOwner = { nullptr };
	Engine::CMapObject*			pStaticSource = { nullptr };
}CUTSCENE_OBJECT_RUNTIME;

// 컷신 에셋 하나에 속한 사전 복제 오브젝트 집합이다.
typedef struct tagCutScenePreparedAsset
{
	unordered_map<_uint, CUTSCENE_OBJECT_RUNTIME> Objects;
}CUTSCENE_PREPARED_ASSET;

class GAMECORE_DLL CCutScene_Manager final : public CBase, public Engine::IListener
{
private:
	CCutScene_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CCutScene_Manager() = default;

public:
	HRESULT Initialize(const function<_bool()>& CanPlayCallback, const function<void(_bool)>& PlaybackStateCallback, const function<_bool(const string&)>& PlaybackFinishedCallback);
	HRESULT Preload(const string& strAssetName);
	HRESULT Prepare_Chapter(_uint iObjectLevel, _uint iModelLevel);
	void Release_Chapter();
	HRESULT Play(const CUTSCENE_ASSET& Asset);
	HRESULT Play(const string& strAssetName);
	void Update(_float fTimeDelta);
	void Pre_MapUpdate();
	void Post_MapUpdate();
	void Stop();
	void Clear_Cache();
	virtual void OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

public:
	_bool Is_Playing() const { return CUTSCENE_PLAY_STATE::PLAYING == m_ePlayState; }
	_float Get_CurrentTime() const { return m_fCurrentTime; }

private:
	HRESULT Ready_RuntimeCamera();
	HRESULT Ready_ScreenOverlay();
	HRESULT Start_Play(const CUTSCENE_ASSET& Asset, const string& strAssetName);
	void Handle_PlayRequest(Event_Desc* pData, _bool bInteractionRequest);
	void Set_PlayState(CUTSCENE_PLAY_STATE ePlayState);
	void Suppress_HUDLayer();
	void Restore_HUDLayer();
	HRESULT Prepare_Asset(const string& strAssetName, const CUTSCENE_ASSET& Asset);
	void Release_PreparedAssets();
	void Bind_RuntimeAttachments(const CUTSCENE_ASSET& Asset, CUTSCENE_PREPARED_ASSET* pPreparedAsset);
	void Sort_CutScene();
	void Evaluate_Camera(_float fTime);
	void Evaluate_Objects(_float fTime, _bool bSeek);
	void Evaluate_RuntimePlayer(const CUTSCENE_OBJECT_TRACK& Track, _float fTime, _bool bSeek);
	void Evaluate_Screen(_float fTime);
	void Dispatch_Sounds(_float fPreviousTime, _float fCurrentTime);
	void Start_SoundEvent(const CUTSCENE_SOUND_EVENT& Event);
	void Stop_SoundEvent(const CUTSCENE_SOUND_EVENT& Event);
	void Stop_Sounds();
	void Dispatch_Effects(_float fPreviousTime, _float fCurrentTime);
	void Start_EffectEvent(const CUTSCENE_EFFECT_EVENT& Event);
	void Stop_EffectEvent(const CUTSCENE_EFFECT_EVENT& Event);
	void Stop_Effects();
	void Reset_RuntimePlayer();
	void Reset_RuntimeObjects();
	void Restore_InstanceOverrides();
	void Stop_CutSceneCamera();
	void Reset_ScreenOverlay();
	void Finish_Playback();
	HRESULT Load_Asset(const string& strAssetName, CUTSCENE_ASSET* pOutAsset);
	HRESULT Prepare_EffectResources(const CUTSCENE_ASSET& Asset);
	CUTSCENE_PREPARED_ASSET* Find_ActivePreparedAsset();
	CCutScene_Actor* Find_Actor(CUTSCENE_PREPARED_ASSET* pPreparedAsset, _uint iObjectID) const;
	const _float4x4* Find_ObjectWorld(_uint iObjectID) const;
	Engine::CMapInstanceObject* Resolve_InstanceOwner(const CUTSCENE_INSTANCE_SOURCE& Source) const;
	Engine::CMapObject* Resolve_StaticSource(const CUTSCENE_INSTANCE_SOURCE& Source) const;

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	Engine::CGameInstance* m_pGameInstance = { nullptr };
	CCutScene_Camera* m_pCutSceneCamera = { nullptr };
	Engine::CUIElement* m_pColorOverlay = { nullptr };
	Engine::CUIElement* m_pFadeOverlay = { nullptr };
	UI_HANDLE m_hScreenOverlay = {};

	CUTSCENE_ASSET m_Asset = {};
	unordered_map<string, CUTSCENE_ASSET> m_CachedAssets = {};
	// 재생 Stop은 활성 Effect만 정리하고, 같은 키와 파일은 다음 재생에서 다시 Load하지 않는다.
	unordered_map<string, _wstring> m_LoadedEffectSourcePaths = {};
	unordered_map<string, CUTSCENE_PREPARED_ASSET> m_PreparedAssets = {};
	string m_strActiveAssetName = {};
	_uint m_iObjectLevel = { UINT_MAX };
	_uint m_iModelLevel = { UINT_MAX };
	vector<Engine::CMapObject*> m_TemporarilyHiddenInstances = {};
	vector<pair<Engine::CMapInstanceObject*, _uint>> m_TemporarilyHiddenRuntimeInstances = {};
	unordered_set<Engine::CMapInstanceObject*> m_InstanceOverrideOwners = {};
	unordered_map<_uint, _uint> m_SoundEventHandles = {};
	unordered_map<_uint, Engine::CEffect*> m_EffectEventHandles = {};
	CUTSCENE_PLAY_STATE m_ePlayState = { CUTSCENE_PLAY_STATE::STOPPED };
	_double m_dPlaybackTime = {};
	_float m_fCurrentTime = {};
	_uint m_iActiveCameraSectionID = { UINT_MAX };
	_bool m_bPlaybackCleanupPending = { false };
	_bool m_bHUDLayerSuppressed = { false };
	function<_bool()> m_CanPlayCallback = { nullptr };
	function<void(_bool)> m_PlaybackStateCallback = { nullptr };
	function<_bool(const string&)> m_PlaybackFinishedCallback = { nullptr };

public:
	static CCutScene_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const function<_bool()>& CanPlayCallback, const function<void(_bool)>& PlaybackStateCallback, const function<_bool(const string&)>& PlaybackFinishedCallback);
	virtual void Free() override;
};

NS_END

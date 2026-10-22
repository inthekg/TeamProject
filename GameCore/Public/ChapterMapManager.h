#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ChapterMapData.h"

NS_BEGIN(Engine)
class CMap;
class CMapController;
class CGameInstance;
class CNavigation;
NS_END

NS_BEGIN(GameCore)

class CMapWorld;
class CMonsterSpawnManager;
class CMapGameplayObjectManager;
class CActorSectorManager;
class CMapLightManager;
class CNavigationManager;
class CMapTriggerManager;
class CMapFogManager;
class CMapEffectManager;
class CStargazerManager;
class CMapWaterManager;

class GAMECORE_DLL CChapterMapManager final : public CBase
{
private:
	CChapterMapManager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CChapterMapManager() = default;

//추가 
public:
	static HRESULT		Get_ChapterPath(_uint iChapterIndex, _wstring& OutPath);

public:
	//현재 챕터 철거. 단, 인스턴스 객체만 해제한다. 프로토타입 등은 재방문 대비하여 남겨둔다 
	HRESULT				Unload_Chapter();
	//사망시 리셋. 맵 초기화가 아닌 리스폰 
	HRESULT				Reset_Chapter();
	HRESULT				Refresh_CurrentSector(_fvector vPosition);

public:
	_bool				Is_ChapterLoaded() const { return m_pMapWorld != nullptr; }
	const _wstring&		Get_CurrentChapterPath() const { return m_strCurrentChapterPath; }
//추가 끝 

public:
	HRESULT				Load_Chapter(const _wstring& strChapterPath, LEVEL eLevel = LEVEL::GAMEPLAY, LEVEL eProtoLevel = LEVEL::GAMEPLAY);
	// Logo는 World Map과 비상호작용 장식물만 준비하는 경량 Chapter 경로를 사용한다.
	HRESULT				Load_LogoChapter(const _wstring& strChapterPath);
	void				Update(_fvector vTrackingPosition, _fvector vCameraPosition);
	void				Late_Update(_float fTimeDelta);
	void				Get_CutSceneAssetNames(vector<string>* pOutAssetNames) const;
	HRESULT				Resolve_StargazerTravel(_uint64 iTargetStargazerID, _uint iCurrentChapterIndex, _uint iCurrentSectorID, Respawn_Point* pOutPoint) const;

	HRESULT				Spawn_All();
	HRESULT				Spawn_StartSector();
	HRESULT				Spawn_MapGameplayObjects();

public:
	Engine::CMap*		Get_CurrentMap() const;
	Engine::CMapController* Get_MapController() const;

public :
	_uint Get_CurrentSectorID() const;
	_uint Find_PrimarySectorByPosition(_fvector vPosition) const;
	Engine::CNavigation* Find_Navigation(_uint iSectorID) const;


private:
	//추가 
	void				Release_SubManagers();
	//추가 끝 

private:
	ID3D11Device*			m_pDevice = nullptr;
	ID3D11DeviceContext*	m_pContext = nullptr;
	Engine::CGameInstance*	m_pGameInstance = nullptr;

	CHAPTER_DESC			m_ChapterDesc = {};
	CMapWorld*				m_pMapWorld = nullptr;
	CMonsterSpawnManager*	m_pMonsterSpawnManager = nullptr;
	CMapGameplayObjectManager* m_pMapGameplayObjectManager = nullptr;
	CActorSectorManager*	m_pActorSectorManager = nullptr;
	CMapLightManager*		m_pMapLightManager = nullptr;
	CNavigationManager*		m_pNavigationManager = nullptr;
	CMapTriggerManager*		m_pMapTriggerManager = nullptr;
	CMapFogManager*			m_pMapFogManager = nullptr;
	CMapEffectManager*		m_pMapEffectManager = nullptr;
	CStargazerManager*		m_pStargazerManager = nullptr;
	CMapWaterManager*		m_pMapWaterManager = nullptr;

	//추가
	_wstring				m_strCurrentChapterPath = {};
	LEVEL					m_eLevel = { LEVEL::GAMEPLAY };
	_uint					m_iCurrentChapterIndex = { UINT_MAX };
		
	static const vector<_wstring> g_ChapterLayerTags; // 챕터 소속 레이어. Unload시 여기있는 레이어만 해제함 
	//추가끝 
public:
	static CChapterMapManager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END

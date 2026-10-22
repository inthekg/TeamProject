#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ChapterMapData.h"


NS_BEGIN(Engine)
class CMap;
class CMapController;
class CGameInstance;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CMapWorld final : public CBase
{
    // 챕터의 섹터 맵, 렌더 활성 범위, 링크 전환, 맵 피직스 활성 범위를 관리한다.
private:
    CMapWorld(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual  ~CMapWorld() = default;

public:
    _uint Get_CurrentSectorID() const { return m_iCurrentSectorID; }
    _uint Get_PreviousSetorId() const { return m_iPreviousSectorID; }
    Engine::CMap* Get_CurrentMap() const;
    const unordered_set<_uint>& Get_ActiveRenderSectorIDs() const { return m_ActiveRenderSectorIDs; }
    const unordered_set<_uint>& Get_ActiveSimulationSectorIDs() const { return m_ActiveSimulationSectorIDs; }
    Engine::CMapController* Get_MapController() const { return m_pMapController; }
    Engine::CMap* Get_HLODMap(_uint iSectorID) const
    {
        auto Iter = m_LoadedHLODMaps.find(iSectorID);
        return (m_LoadedHLODMaps.end() == Iter) ? nullptr : Iter->second;
    }


public:
    HRESULT Initialize(const CHAPTER_DESC& ChapterDesc);
    void Update(_fvector vPlayerPosition, _fvector vCameraPosition);
    void Late_Update(_float fTimeDelta);


    // 초기화/텔레포트처럼 강제 보정이 필요한 순간에 위치 기준 primary 섹터를 한 번 찾는다.
    HRESULT Refresh_CurrentSectorByPosition(_fvector vTrackingPosition);

 
public:
    // 로드된 모든 섹터 맵 오브젝트를 지정 레이어에 등록한다.
    void Add_GameObjects_ToMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag);
    _uint Find_PrimarySectorByPosition(_fvector vPosition) const;

private:
    // 챕터 데이터에 있는 섹터 ID와 로드된 맵 포인터를 연결해 캐싱한다.
    HRESULT Load_AllSectors();
    HRESULT Register_AllSectors();
    HRESULT Register_CurrentSector();
    HRESULT Prepare_ActiveRenderResources();

    // 현재 섹터 기준으로 렌더링할 섹터 집합을 갱신한다.
    HRESULT Update_ActiveSectors();
    void Update_ActiveSimulationSectors(_bool bIncludePreviousSector);

    HRESULT Register_RenderSector(_uint iSectorID);
    void Unregister_RenderSector(_uint iSectorID);
    unordered_set<_uint> Collect_NeighborSectorIDs(_uint iCurrentSectorID, _bool bTransitionOnly) const;

    _bool Set_CurrentSector(_uint iSectorID);


    _float Calc_DistanceToLink(const SECTOR_LINK_DESC& Link, _fvector vPosition) const;
    _float Calc_DistanceToSectorBounds(const BoundingBox& Bounds, _fvector vCameraPosition) const;
    void Set_HLODRenderState(_uint iSectorID, _bool bUseHLOD);
    void Reset_HLODRenderStates();
    void Update_HLODRenderStates(_fvector vCameraPosition);

    void Update_Link(_fvector vPosition);

    void Change_Sector(const SECTOR_LINK_DESC& Link);

    void Update_ActiveRenderLinks(_fvector vCameraPosition);
    void Apply_SectorRenderPolicies(_fvector vCameraPosition);


#ifdef _DEBUG
    void Update_MapPhysicsDebugVisibility();
    void Reset_MapPhysicsDebugVisibility();

#endif // _DEBUG


private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    Engine::CGameInstance* m_pGameInstance = nullptr;
    Engine::CMapController* m_pMapController = nullptr;

    CHAPTER_DESC m_ChapterDesc;
    unordered_map<_uint, Engine::CMap*> m_LoadedMaps;
    unordered_map<_uint, Engine::CMap*> m_LoadedHLODMaps;
    unordered_set<_uint> m_RegisteredSectorIDs;
    unordered_set<_uint> m_ActiveRenderSectorIDs;
    unordered_set<_uint> m_ActiveSimulationSectorIDs;
    unordered_set<_uint> m_ActiveRenderLinkIDs;

    unordered_set<_uint> m_HLODRenderSectorIDs;

    _uint m_iCurrentSectorID = UINT_MAX;
    _uint m_iPreviousSectorID = UINT_MAX;

    _float3 m_vPreviousPosition{};
    _bool m_bHasPreviousPosition = false;
    _bool m_bTransitionLocked = false;
    BoundingBox m_TransitionLockBounds{};

    _float m_fHLODEnterDistance = 75.f;
    _float m_fHLODExitDistance = 50.f;

#ifdef _DEBUG
    unordered_set<_wstring> m_VisiblePhysicsDebugMaps;
#endif

public:
    static CMapWorld* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc);
    virtual void Free() override;

};

NS_END

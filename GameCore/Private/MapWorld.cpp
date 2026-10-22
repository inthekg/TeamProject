#include "MapWorld.h"
#include "GameInstance.h" 
#include "Map.h"
#include "MapController.h"

static _matrix Make_LinkRotationMatrix(const SECTOR_LINK_DESC& Link)
{
    // 에디터 입력값은 degree 기준이라 라디안으로 변환해서 회전 행렬을 만든다.
    return XMMatrixRotationRollPitchYaw(XMConvertToRadians(Link.Rotation.x), XMConvertToRadians(Link.Rotation.y), XMConvertToRadians(Link.Rotation.z));
}

static _vector Get_LinkWorldForward(const SECTOR_LINK_DESC& Link)
{
    _vector vForward = XMLoadFloat3(&Link.Forward);

    if (XMVectorGetX(XMVector3LengthSq(vForward)) <= FLT_EPSILON)
        vForward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        vForward = XMVector3Normalize(vForward);

    // 링크 로컬 방향으로 저장된 Forward를 링크 회전값까지 반영한 월드 방향으로 변환한다.
    return XMVector3Normalize(XMVector3TransformNormal(vForward, Make_LinkRotationMatrix(Link)));
}

static _bool Intersects_RotatedLinkBounds(const SECTOR_LINK_DESC& Link, _fvector vRayOrigin, _fvector vRayDirection, _float* pOutDistance)
{
    if (nullptr == pOutDistance)
        return false;

    // 월드 레이를 링크 로컬 공간으로 옮긴 뒤 일반 AABB 교차 검사로 처리한다.
    _matrix matInvRotation = XMMatrixTranspose(Make_LinkRotationMatrix(Link));
    _vector vLocalOrigin = XMVector3TransformCoord(vRayOrigin - XMLoadFloat3(&Link.Bounds.Center), matInvRotation);
    _vector vLocalDirection = XMVector3TransformNormal(vRayDirection, matInvRotation);

    if (XMVectorGetX(XMVector3LengthSq(vLocalDirection)) <= FLT_EPSILON)
        return false;

    vLocalDirection = XMVector3Normalize(vLocalDirection);

    BoundingBox LocalBounds{};
    LocalBounds.Center = _float3(0.f, 0.f, 0.f);
    LocalBounds.Extents = Link.Bounds.Extents;

    return LocalBounds.Intersects(vLocalOrigin, vLocalDirection, *pOutDistance);
}

CMapWorld::CMapWorld(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

Engine::CMap* CMapWorld::Get_CurrentMap() const
{
    // 현재 섹터 ID에 대응되는 캐싱된 맵을 찾는다.
    auto Iter = m_LoadedMaps.find(m_iCurrentSectorID);
    return Iter == m_LoadedMaps.end() ? nullptr : Iter->second;
}

HRESULT CMapWorld::Initialize(const CHAPTER_DESC& ChapterDesc)
{
    // 섹터 데이터가 없거나 시작 섹터가 유효하지 않으면 월드를 구성할 수 없다.
    if (ChapterDesc.Sectors.empty())
        return E_FAIL;

    if (ChapterDesc.Sectors.find(ChapterDesc.StartSectorID) == ChapterDesc.Sectors.end())
        return E_FAIL;

    m_ChapterDesc = ChapterDesc;
    m_iCurrentSectorID = ChapterDesc.StartSectorID;
    m_iPreviousSectorID = UINT_MAX;

    if (FAILED(Load_AllSectors()))
        return E_FAIL;

    m_pMapController = CMapController::Create(m_pDevice, m_pContext);

    if (nullptr == m_pMapController)
        return E_FAIL;

    // 전체 섹터 등록 방식이 필요하면 Register_AllSectors를 다시 사용한다.
    if (FAILED(Register_AllSectors()))
        return E_FAIL;

    if (FAILED(Update_ActiveSectors()))
        return E_FAIL;

    // 첫 렌더 프레임에서 생성되던 전체 인스턴스 공용 GPU 버퍼를 로딩 중 준비한다.
    if (FAILED(m_pMapController->Prepare_GlobalRenderResources()))
        return E_FAIL;

    if (FAILED(Prepare_ActiveRenderResources()))
        return E_FAIL;

    // 로딩 중 생성한 GPU 리소스 명령을 실제 렌더 시작 전에 드라이버로 넘긴다.
    m_pContext->Flush();

#ifdef _DEBUG
    Update_MapPhysicsDebugVisibility();
#endif // _DEBUG


    Update_ActiveSimulationSectors(false);

    auto CurrentIter = m_ChapterDesc.Sectors.find(m_iCurrentSectorID);

    if (m_ChapterDesc.Sectors.end() != CurrentIter)
    {
        // 첫 프레임 전에 현재 섹터만 최소한 보이도록 초기 렌더 정책을 한 번 적용한다.
        Apply_SectorRenderPolicies(XMLoadFloat3(&CurrentIter->second.Bounds.Center));
    }

	return S_OK;
}

void CMapWorld::Update(_fvector vPlayerPosition, _fvector vCameraPosition)
{
    // 링크 전환, 활성 섹터, 포탈 렌더 정책을 현재 추적 위치 기준으로 갱신한다.
    Update_Link(vPlayerPosition);
    //Update_ActiveSectors();
    Update_ActiveRenderLinks(vCameraPosition);
    Apply_SectorRenderPolicies(vCameraPosition);
    Update_HLODRenderStates(vCameraPosition);
}

void CMapWorld::Late_Update(_float fTimeDelta)
{
    // MapController는 레이어가 아니라 MapWorld 소유로 최신 카메라 기준 컬링/렌더 제출만 수행한다.
    m_pMapController->Late_Update(fTimeDelta);
}

HRESULT CMapWorld::Refresh_CurrentSectorByPosition(_fvector vTrackingPosition)
{
    if (UINT_MAX == m_iCurrentSectorID)
        return E_FAIL;

    const _uint iOldSectorID = m_iCurrentSectorID;
    const _uint iFoundSectorID = Find_PrimarySectorByPosition(vTrackingPosition);

    if (UINT_MAX != iFoundSectorID)
        Set_CurrentSector(iFoundSectorID);

    _float3 vTrackingPositionValue{};
    XMStoreFloat3(&vTrackingPositionValue, vTrackingPosition);
    m_vPreviousPosition = vTrackingPositionValue;
    m_bHasPreviousPosition = true;
    m_bTransitionLocked = false;

    m_ActiveRenderLinkIDs.clear();

    // 시작·리스폰·텔레포트는 현재 섹터만 Simulation한다.
    Update_ActiveSimulationSectors(false);

    if (FAILED(Update_ActiveSectors()))
        return E_FAIL;

#ifdef _DEBUG
    Update_MapPhysicsDebugVisibility();
#endif

    // 강제 보정 시점에는 카메라가 아직 이동 전일 수 있으므로 우선 플레이어 위치로 맞춘다.
    Update_ActiveRenderLinks(vTrackingPosition);
    Apply_SectorRenderPolicies(vTrackingPosition);

    Reset_HLODRenderStates();


    if (nullptr != m_pGameInstance)
    {
        // 같은 섹터 리스폰도 이전 Simulation 섹터 제거를 전파해야 한다.
        SECTOR_CHANGED_EVENT_DESC EventDesc{};
        EventDesc.iPreviousSectorID = iOldSectorID;
        EventDesc.iCurrentSectorID = m_iCurrentSectorID;
        EventDesc.pActiveRenderSectorIDs = &m_ActiveRenderSectorIDs;
        EventDesc.pActiveSimulationSectorIDs = &m_ActiveSimulationSectorIDs;
        m_pGameInstance->Broadcast(EVENT_TYPE::SECTOR_CHANGED, &EventDesc);
    }

    return S_OK;
}


void CMapWorld::Add_GameObjects_ToMgr(_uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    // 로드된 모든 섹터 맵의 게임 오브젝트를 지정 레이어에 등록한다.
    for (const auto& Pair : m_LoadedMaps)
    {
        Engine::CMap* pMap = Pair.second;

        if (nullptr == pMap)
            continue;

        pMap->Add_GameObjects_ToMgr(iLayerLevelIndex, strLayerTag);
    }
}

//HRESULT CMapWorld::Load_Metadata(const _wstring& strMetadataPath)
//{
//	return E_NOTIMPL;
//}

HRESULT CMapWorld::Load_AllSectors()
{
    m_LoadedMaps.clear();
    m_LoadedHLODMaps.clear();
    m_HLODRenderSectorIDs.clear();

    // 챕터 섹터 데이터의 맵 이름으로 CMap 포인터를 찾아 섹터 ID에 묶어 둔다.
    for (const auto& Pair : m_ChapterDesc.Sectors)
    {
        const SECTOR_DESC& SectorDesc = Pair.second;
        Engine::CMap* pOriginalMap = m_pGameInstance->Get_Map(SectorDesc.MapName);

        if (nullptr == pOriginalMap)
            return E_FAIL;

        m_LoadedMaps.emplace(SectorDesc.ID, pOriginalMap);

        const _wstring strHLODMapName = SectorDesc.MapName + L"_HLOD";
        Engine::CMap* pHLODMap = m_pGameInstance->Get_Map(strHLODMapName);

        // HLOD 없어도 실패 처리는 안함
        if (nullptr != pHLODMap)
            m_LoadedHLODMaps.emplace(SectorDesc.ID,pHLODMap);
    }

    return S_OK;
}

HRESULT CMapWorld::Register_CurrentSector()
{
    return Register_RenderSector(m_iCurrentSectorID);
}

HRESULT CMapWorld::Update_ActiveSectors()
{
    if (UINT_MAX == m_iCurrentSectorID)
        return E_FAIL;

    // 현재 primary와 모든 링크 이웃을 Render 후보로 구성한다.
    unordered_set<_uint> DesiredSectorIDs = Collect_NeighborSectorIDs(m_iCurrentSectorID, false);
    DesiredSectorIDs.emplace(m_iCurrentSectorID);

    for (const auto& Pair : m_ChapterDesc.Sectors)
    {
        const SECTOR_DESC& Sector = Pair.second;

        if (SECTOR_ROLE::SUPPORT != Sector.eRole)
            continue;

        for (_uint iOwnerSectorID : Sector.OwnerSectorIDs)
        {
            // 활성 Owner를 가진 SUPPORT도 Render 후보에 포함한다.
            if (DesiredSectorIDs.end() != DesiredSectorIDs.find(iOwnerSectorID))
            {
                DesiredSectorIDs.emplace(Sector.ID);
                break;
            }
        }
    }

    if (nullptr != m_pMapController)
    {
        for (_uint iSectorID : m_ActiveRenderSectorIDs)
        {
            // 이전 Render 집합에서 빠진 섹터는 HIDDEN 처리한다.
            if (DesiredSectorIDs.end() == DesiredSectorIDs.find(iSectorID))
                m_pMapController->Set_SectorRenderHidden(iSectorID);
        }
    }

    m_ActiveRenderSectorIDs = move(DesiredSectorIDs);

    return S_OK;
}

void CMapWorld::Update_ActiveSimulationSectors(_bool bIncludePreviousSector)
{
    m_ActiveSimulationSectorIDs.clear();

    if (UINT_MAX != m_iCurrentSectorID)
        m_ActiveSimulationSectorIDs.emplace(m_iCurrentSectorID);

    // 정상 링크 전환에서만 이전+현재를 동시에 Simulation한다.
    if (bIncludePreviousSector && UINT_MAX != m_iPreviousSectorID)
        m_ActiveSimulationSectorIDs.emplace(m_iPreviousSectorID);
}

void CMapWorld::Update_ActiveRenderLinks(_fvector vTrackingPosition)
{
    for (const SECTOR_LINK_DESC& Link : m_ChapterDesc.Links)
    {
        // 전환 전용 링크는 렌더 포탈 대상으로 쓰지 않는다.
        if (Link.eType == SECTOR_LINK_TYPE::TRANSITION_ONLY) 
        {
            m_ActiveRenderLinkIDs.erase(Link.ID); 
            continue;
        }

        const _bool bConnectedToCurrent = Link.SourceID == m_iCurrentSectorID || (Link.TwoWay && Link.TargetID == m_iCurrentSectorID);

        // 현재 섹터와 연결되지 않은 링크는 렌더 활성 상태에서 제외한다.
        if (false == bConnectedToCurrent)
        {
            m_ActiveRenderLinkIDs.erase(Link.ID);
            continue;
        }

        const _float fDistance = Calc_DistanceToLink(Link, vTrackingPosition);
        const _bool bActive =
            m_ActiveRenderLinkIDs.end() != m_ActiveRenderLinkIDs.find(Link.ID);

        // Enter/Exit 거리로 히스테리시스를 두어 포탈 렌더 깜빡임을 줄인다.
        if (false == bActive && fDistance <= Link.RenderEnter)
        {
            m_ActiveRenderLinkIDs.emplace(Link.ID);
        }
        else if (bActive && fDistance > Link.RenderExit)
        {
            m_ActiveRenderLinkIDs.erase(Link.ID);

        }
    }
}

void CMapWorld::Apply_SectorRenderPolicies(_fvector vCameraPosition)
{
    if (nullptr == m_pMapController)
        return;

    auto AppendOwnerPortalDescs = [this, vCameraPosition](_uint iOwnerSectorID, vector<Engine::CMapController::tSectorPortalRenderDesc>& OutPortalRenderDescs)
        {
            for (const SECTOR_LINK_DESC& Link : m_ChapterDesc.Links)
            {
                if (SECTOR_LINK_TYPE::TRANSITION_ONLY == Link.eType)
                    continue;

                if (m_ActiveRenderLinkIDs.end() == m_ActiveRenderLinkIDs.find(Link.ID))
                    continue;

                const _bool bForward = Link.SourceID == m_iCurrentSectorID && Link.TargetID == iOwnerSectorID;
                const _bool bBackward = Link.TwoWay && Link.TargetID == m_iCurrentSectorID && Link.SourceID == iOwnerSectorID;

                if (false == bForward && false == bBackward)
                    continue;

                Engine::CMapController::tSectorPortalRenderDesc PortalDesc{};
                _float3 vLinkForward{};
                XMStoreFloat3(&vLinkForward, Get_LinkWorldForward(Link));

                PortalDesc.BoundingPortal = Link.Bounds;
                PortalDesc.vRotation = Link.Rotation;
                PortalDesc.vDirection = vLinkForward;
                PortalDesc.bUseDepth = SECTOR_LINK_TYPE::TRANSITION_AND_RENDER == Link.eType;
                PortalDesc.fViewDepth = Link.RenderDepth >= 0.f ? Link.RenderDepth : Link.RenderExit;

                const _float fDistance = Calc_DistanceToLink(Link, vCameraPosition);
                const _float fExpandDistance = 1.f;
                _float fNearRatio = 1.f - min(1.f, fDistance / fExpandDistance);
                fNearRatio = fNearRatio * fNearRatio * fNearRatio;
                PortalDesc.fSidePadding = PortalDesc.bUseDepth ? 0.15f + fNearRatio * 0.9f : 0.f;

                if (bBackward)
                {
                    PortalDesc.vDirection.x *= -1.f;
                    PortalDesc.vDirection.y *= -1.f;
                    PortalDesc.vDirection.z *= -1.f;
                }

                OutPortalRenderDescs.push_back(PortalDesc);
            }
        };

    for (_uint iSectorID : m_ActiveRenderSectorIDs)
    {
        auto SectorIter = m_ChapterDesc.Sectors.find(iSectorID);

        if (m_ChapterDesc.Sectors.end() == SectorIter)
            continue;

        const SECTOR_DESC& Sector = SectorIter->second;

        if (SECTOR_ROLE::PRIMARY == Sector.eRole && iSectorID == m_iCurrentSectorID)
        {
            m_pMapController->Set_SectorRenderFull(iSectorID);
            continue;
        }

        if (SECTOR_ROLE::SUPPORT == Sector.eRole)
        {
            _bool bOwnedByCurrentSector = false;

            for (_uint iOwnerSectorID : Sector.OwnerSectorIDs)
            {
                if (iOwnerSectorID == m_iCurrentSectorID)
                {
                    bOwnedByCurrentSector = true;
                    break;
                }
            }

            // 현재 Owner의 SUPPORT는 Owner의 FULL 정책을 공유한다.
            if (bOwnedByCurrentSector)
            {
                m_pMapController->Set_SectorRenderFull(iSectorID);
                continue;
            }
        }

        vector<Engine::CMapController::tSectorPortalRenderDesc> PortalRenderDescs{};

        if (SECTOR_ROLE::PRIMARY == Sector.eRole)
        {
            AppendOwnerPortalDescs(iSectorID, PortalRenderDescs);
        }
        else
        {
            // 이웃 Owner의 SUPPORT는 해당 Owner의 PortalDesc를 공유한다.
            for (_uint iOwnerSectorID : Sector.OwnerSectorIDs)
            {
                if (m_ActiveRenderSectorIDs.end() == m_ActiveRenderSectorIDs.find(iOwnerSectorID))
                    continue;

                AppendOwnerPortalDescs(iOwnerSectorID, PortalRenderDescs);
            }
        }

        if (PortalRenderDescs.empty())
            m_pMapController->Set_SectorRenderHidden(iSectorID);
        else
            m_pMapController->Set_SectorPortalRender(iSectorID, PortalRenderDescs);
    }
}

#ifdef _DEBUG
void CMapWorld::Update_MapPhysicsDebugVisibility()
{
    if (nullptr == m_pGameInstance)
        return;

    unordered_set<_wstring> VisibleMaps{};

    for (_uint iSectorID : m_ActiveRenderSectorIDs)
    {
        auto SectorIter = m_ChapterDesc.Sectors.find(iSectorID);

        if (m_ChapterDesc.Sectors.end() == SectorIter)
            continue;

        // 현재 또는 주변 섹터에 속한 맵 이름을 표시 대상으로 수집한다.
        VisibleMaps.emplace(SectorIter->second.MapName);
    }

    // 같은 섹터 구성이라면 PhysX Actor 목록을 다시 순회하지 않는다.
    if (VisibleMaps == m_VisiblePhysicsDebugMaps)
        return;

    unordered_set<_wstring> ProcessedMaps{};

    for (const auto& Pair : m_ChapterDesc.Sectors)
    {
        const _wstring& strMapName = Pair.second.MapName;

        // 동일한 맵을 여러 섹터가 사용해도 맵당 한 번만 처리한다.
        if (false == ProcessedMaps.emplace(strMapName).second)
            continue;

        const _bool bVisible = VisibleMaps.end() != VisibleMaps.find(strMapName);

        // 충돌과 Scene 등록은 유지하고 디버그 표시 플래그만 변경한다.
        m_pGameInstance->Set_MapPhysicsDebugVisible(strMapName, bVisible);
    }

    m_VisiblePhysicsDebugMaps = move(VisibleMaps);
}

void CMapWorld::Reset_MapPhysicsDebugVisibility()
{
    if (nullptr == m_pGameInstance)
        return;

    unordered_set<_wstring> ProcessedMaps{};

    for (const auto& Pair : m_ChapterDesc.Sectors)
    {
        const _wstring& strMapName = Pair.second.MapName;

        if (false == ProcessedMaps.emplace(strMapName).second)
            continue;

        // 캐시된 맵이 다른 월드에서 재사용될 수 있으므로 기본 표시 상태로 복원한다.
        m_pGameInstance->Set_MapPhysicsDebugVisible(strMapName, true);
    }

    m_VisiblePhysicsDebugMaps.clear();
}
#endif

HRESULT CMapWorld::Register_RenderSector(_uint iSectorID)
{
    // 이미 등록된 섹터는 Original 공간 구조를 중복 생성하지 않는다.
    if (m_RegisteredSectorIDs.end() != m_RegisteredSectorIDs.find(iSectorID))
        return S_OK;

    if (nullptr == m_pMapController)
        return E_FAIL;

    auto OriginalIter = m_LoadedMaps.find(iSectorID);

    if (m_LoadedMaps.end() == OriginalIter || nullptr == OriginalIter->second)
        return E_FAIL;

    Engine::CMap* pHLODMap = nullptr;
    auto HLODIter = m_LoadedHLODMaps.find(iSectorID);

    if (m_LoadedHLODMaps.end() != HLODIter)
        pHLODMap = HLODIter->second;

    // HLOD가 없는 섹터는 nullptr을 전달하고 Original만 등록한다.
    if (FAILED(m_pMapController->Register_Sector(iSectorID, OriginalIter->second, pHLODMap)))
        return E_FAIL;

    m_RegisteredSectorIDs.emplace(iSectorID);

    return S_OK;
}

void CMapWorld::Unregister_RenderSector(_uint iSectorID)
{
    // 현재 섹터는 실수로 제거되지 않도록 보호한다.
    if (iSectorID == m_iCurrentSectorID)
        return;

    auto Iter = m_RegisteredSectorIDs.find(iSectorID);

    if (m_RegisteredSectorIDs.end() == Iter)
        return;

    if (nullptr != m_pMapController)
        m_pMapController->Unregister_Sector(iSectorID);

    // 요청된 섹터만 각 관리 집합에서 제거한다.
    m_RegisteredSectorIDs.erase(Iter);
    m_ActiveRenderSectorIDs.erase(iSectorID);
    m_ActiveSimulationSectorIDs.erase(iSectorID);
    m_HLODRenderSectorIDs.erase(iSectorID);
}


unordered_set<_uint> CMapWorld::Collect_NeighborSectorIDs(_uint iCurrentSectorID, _bool bTransitionOnly) const
{
    unordered_set<_uint> NeighborSectorIDs;

    // 렌더는 모든 링크 이웃을 쓰고, 피직스는 이동 가능한 링크 이웃만 쓴다.
    for (const SECTOR_LINK_DESC& Link : m_ChapterDesc.Links)
    {
        if (bTransitionOnly && SECTOR_LINK_TYPE::RENDER_ONLY == Link.eType)
            continue;

        if (Link.SourceID == iCurrentSectorID)
        {
            if (UINT_MAX != Link.TargetID)
                NeighborSectorIDs.emplace(Link.TargetID);

            continue;
        }

        if (Link.TwoWay && Link.TargetID == iCurrentSectorID)
        {
            if (UINT_MAX != Link.SourceID)
                NeighborSectorIDs.emplace(Link.SourceID);
        }
    }

    return NeighborSectorIDs;
}

_uint CMapWorld::Find_PrimarySectorByPosition(_fvector vPosition) const
{
    auto CurrentIter = m_ChapterDesc.Sectors.find(m_iCurrentSectorID);

    if (m_ChapterDesc.Sectors.end() != CurrentIter)
    {
        const SECTOR_DESC& CurrentSector = CurrentIter->second;

        // 현재 primary bounds 안에 그대로 있으면 섹터를 흔들지 않는다.
        if (SECTOR_ROLE::PRIMARY == CurrentSector.eRole && DISJOINT != CurrentSector.Bounds.Contains(vPosition))
            return CurrentSector.ID;
    }

    _uint iNearestSectorID = UINT_MAX;
    _float fNearestDistanceSq = FLT_MAX;

    for (const auto& Pair : m_ChapterDesc.Sectors)
    {
        const SECTOR_DESC& Sector = Pair.second;

        if (SECTOR_ROLE::PRIMARY != Sector.eRole)
            continue;

        if (DISJOINT == Sector.Bounds.Contains(vPosition))
            continue;

        _vector vCenter = XMLoadFloat3(&Sector.Bounds.Center);
        const _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vPosition - vCenter));

        if (fDistanceSq < fNearestDistanceSq)
        {
            fNearestDistanceSq = fDistanceSq;
            iNearestSectorID = Sector.ID;
        }
    }

    return iNearestSectorID;
}

_bool CMapWorld::Set_CurrentSector(_uint iSectorID)
{
    if (UINT_MAX == iSectorID || m_LoadedMaps.end() == m_LoadedMaps.find(iSectorID))
        return false;

    if (m_iCurrentSectorID == iSectorID)
        return false;

    m_iPreviousSectorID = m_iCurrentSectorID;
    m_iCurrentSectorID = iSectorID;

    return true;
}

HRESULT CMapWorld::Register_AllSectors()
{
    if (nullptr == m_pMapController)
        return E_FAIL;

    for (const auto& Pair : m_LoadedMaps)
    {
        // 등록 상태 추적까지 Register_RenderSector()에서 일관되게 처리한다.
        if (FAILED(Register_RenderSector(Pair.first)))
        {
            m_pMapController->Clear_Sectors();
            m_RegisteredSectorIDs.clear();
            return E_FAIL;
        }

        // 로딩 직후 전체 렌더를 막기 위해 기본 상태는 숨김으로 둔다.
        m_pMapController->Set_SectorRenderHidden(Pair.first);
    }

    return S_OK;
}

HRESULT CMapWorld::Prepare_ActiveRenderResources()
{
    if (nullptr == m_pMapController)
        return E_FAIL;

    // 현재 섹터와 링크 이웃 및 연동 Support 섹터만 진입 전에 준비한다.
    for (_uint iSectorID : m_ActiveRenderSectorIDs)
    {
        if (FAILED(m_pMapController->Prepare_SectorRenderResources(iSectorID)))
            return E_FAIL;
    }

    return S_OK;
}

_float CMapWorld::Calc_DistanceToSectorBounds(const BoundingBox& Bounds, _fvector vCameraPosition) const
{
    // 카메라와 Bounds 중심 사이의 축별 절대 거리를 구한다.
    _vector vCameraOffset = XMVectorAbs(XMVectorSetW(vCameraPosition, 0.f) - XMLoadFloat3(&Bounds.Center));

    // Bounds 내부에 포함되는 축은 0, Bounds 밖으로 벗어난 축만 실제 거리로 남긴다.
    _vector vOutsideDistance = XMVectorMax(vCameraOffset - XMLoadFloat3(&Bounds.Extents), XMVectorZero());

    // Bounds 중심이 아니라 표면의 가장 가까운 점까지의 실제 거리를 반환한다.
    return XMVectorGetX(XMVector3Length(vOutsideDistance));
}

void CMapWorld::Set_HLODRenderState(_uint iSectorID, _bool bUseHLOD)
{
    const _bool bCurrentlyUsingHLOD = m_HLODRenderSectorIDs.end() != m_HLODRenderSectorIDs.find(iSectorID);

    // 이미 같은 표현 상태라면 MapController 호출과 컨테이너 작업을 반복하지 않는다.
    if (bCurrentlyUsingHLOD == bUseHLOD)
        return;

    if (false == bUseHLOD)
    {
        if (nullptr != m_pMapController)
            m_pMapController->Set_SectorUseHLOD(iSectorID, false);

        m_HLODRenderSectorIDs.erase(iSectorID);


        return;
    }

    if (nullptr == m_pMapController)
        return;

    auto HLODIter = m_LoadedHLODMaps.find(iSectorID);

    if (m_LoadedHLODMaps.end() == HLODIter || nullptr == HLODIter->second)
        return;

    if (m_pMapController->Set_SectorUseHLOD(iSectorID, true))
    {
        m_HLODRenderSectorIDs.emplace(iSectorID);
    }

}
void CMapWorld::Reset_HLODRenderStates()
{
    if (nullptr != m_pMapController)
    {
        // 현재 HLOD인 섹터만 Original로 되돌려 불필요한 전체 섹터 순회를 피한다.
        for (_uint iSectorID : m_HLODRenderSectorIDs)
            m_pMapController->Set_SectorUseHLOD(iSectorID, false);
    }

    m_HLODRenderSectorIDs.clear();
}

void CMapWorld::Update_HLODRenderStates(_fvector vCameraPosition)
{
    if (nullptr == m_pMapController)
        return;

    // 렌더 후보에서 빠진 섹터와 새 Current Sector는 즉시 Original로 정리한다.
    for (auto Iter = m_HLODRenderSectorIDs.begin(); Iter != m_HLODRenderSectorIDs.end();)
    {
        const _uint iSectorID = *Iter;
        const _bool bInactive = m_ActiveRenderSectorIDs.end() == m_ActiveRenderSectorIDs.find(iSectorID);
        const _bool bCurrentSector = iSectorID == m_iCurrentSectorID;

        if (bInactive || bCurrentSector)
        {
            m_pMapController->Set_SectorUseHLOD(iSectorID, false);
            Iter = m_HLODRenderSectorIDs.erase(Iter);
            continue;
        }

        ++Iter;
    }

    // Primary의 75m 진입과 50m 복귀 상태를 먼저 결정한다.
    for (_uint iSectorID : m_ActiveRenderSectorIDs)
    {
        auto SectorIter = m_ChapterDesc.Sectors.find(iSectorID);

        if (m_ChapterDesc.Sectors.end() == SectorIter)
            continue;

        const SECTOR_DESC& Sector = SectorIter->second;

        if (SECTOR_ROLE::PRIMARY != Sector.eRole)
            continue;

        // 현재 Primary는 거리와 관계없이 항상 Original을 사용한다.
        if (iSectorID == m_iCurrentSectorID)
        {
            Set_HLODRenderState(iSectorID, false);
            continue;
        }

        // HLOD가 없는 Primary는 Original 상태를 유지한다.
        if (m_LoadedHLODMaps.end() == m_LoadedHLODMaps.find(iSectorID))
        {
            Set_HLODRenderState(iSectorID, false);
            continue;
        }

        const _float fDistance = Calc_DistanceToSectorBounds(Sector.Bounds, vCameraPosition);
        const _bool bUsingHLOD = m_HLODRenderSectorIDs.end() != m_HLODRenderSectorIDs.find(iSectorID);

        if (false == bUsingHLOD && fDistance >= m_fHLODEnterDistance)
        {
            // 진입 거리 이상이면 Original에서 HLOD로 변경한다.
            Set_HLODRenderState(iSectorID, true);
        }
        else if (bUsingHLOD && fDistance <= m_fHLODExitDistance)
        {
            // 복귀 거리 이하이면 HLOD에서 Original로 자동 복귀한다.
            Set_HLODRenderState(iSectorID, false);
        }

        // 진입 거리와 복귀 거리 사이에서는 기존 표현 상태를 그대로 유지한다.
    }

    // Support는 자체 거리 대신 활성 Owner의 최종 표현 상태를 따른다.
    for (_uint iSectorID : m_ActiveRenderSectorIDs)
    {
        auto SectorIter = m_ChapterDesc.Sectors.find(iSectorID);

        if (m_ChapterDesc.Sectors.end() == SectorIter)
            continue;

        const SECTOR_DESC& Sector = SectorIter->second;

        if (SECTOR_ROLE::SUPPORT != Sector.eRole)
            continue;

        // Support 자체 HLOD가 없으면 Original 상태를 유지한다.
        if (m_LoadedHLODMaps.end() == m_LoadedHLODMaps.find(iSectorID))
        {
            Set_HLODRenderState(iSectorID, false);
            continue;
        }

        _bool bHasActiveOwner = false;
        _bool bAllActiveOwnersUseHLOD = true;

        for (_uint iOwnerSectorID : Sector.OwnerSectorIDs)
        {
            if (m_ActiveRenderSectorIDs.end() == m_ActiveRenderSectorIDs.find(iOwnerSectorID))
                continue;

            bHasActiveOwner = true;

            // 활성 Owner 중 하나라도 Original이면 Support도 Original을 사용한다.
            if (m_HLODRenderSectorIDs.end() == m_HLODRenderSectorIDs.find(iOwnerSectorID))
            {
                bAllActiveOwnersUseHLOD = false;
                break;
            }
        }

        // 활성 Owner가 모두 HLOD일 때만 Support HLOD를 사용한다.
        Set_HLODRenderState(iSectorID, bHasActiveOwner && bAllActiveOwnersUseHLOD);
    }
}

_float CMapWorld::Calc_DistanceToLink(const SECTOR_LINK_DESC& Link, _fvector vPosition) const
{
    const BoundingBox& Bounds = Link.Bounds;

    // 회전된 링크 박스를 OBB처럼 다루기 위해 위치를 링크 로컬 공간으로 변환한다.
    _matrix matInvRotation = XMMatrixTranspose(Make_LinkRotationMatrix(Link));
    _vector vLocalPosition = XMVector3TransformCoord(vPosition - XMLoadFloat3(&Bounds.Center), matInvRotation);

    _float3 vCurrent{};
    XMStoreFloat3(&vCurrent, vLocalPosition);

    const _float fMinX = -Bounds.Extents.x;
    const _float fMaxX = Bounds.Extents.x;
    const _float fMinY = -Bounds.Extents.y;
    const _float fMaxY = Bounds.Extents.y;
    const _float fMinZ = -Bounds.Extents.z;
    const _float fMaxZ = Bounds.Extents.z;

    const _float fClosestX = max(fMinX, min(vCurrent.x, fMaxX));
    const _float fClosestY = max(fMinY, min(vCurrent.y, fMaxY));
    const _float fClosestZ = max(fMinZ, min(vCurrent.z, fMaxZ));

    const _float fDiffX = vCurrent.x - fClosestX;
    const _float fDiffY = vCurrent.y - fClosestY;
    const _float fDiffZ = vCurrent.z - fClosestZ;

    return sqrtf(fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ);
}



void CMapWorld::Update_Link(_fvector vPosition)
{
    _float3 vCurrent{};
    XMStoreFloat3(&vCurrent, vPosition);

    // 첫 프레임에는 이전 위치 기준점만 저장하고 전환 검사는 건너뛴다.
    if (false == m_bHasPreviousPosition)
    {
        m_vPreviousPosition = vCurrent;
        m_bHasPreviousPosition = true;
        return;
    }

    if (m_bTransitionLocked)
    {
        // 링크 박스 밖으로 완전히 나가기 전까지 같은 링크에서 재전환되지 않게 잠근다.
        const _bool bPreviousOutside = DISJOINT ==
            m_TransitionLockBounds.Contains(XMLoadFloat3(&m_vPreviousPosition));
        const _bool bCurrentOutside = DISJOINT == m_TransitionLockBounds.Contains(vPosition);

        if (bPreviousOutside && bCurrentOutside)
            m_bTransitionLocked = false;

        m_vPreviousPosition = vCurrent;
        return;
    }

    _vector vMove = vPosition - XMLoadFloat3(&m_vPreviousPosition);
    const _float fLength = XMVectorGetX(XMVector3Length(vMove));

    // 이동량이 거의 없으면 링크 통과 검사를 생략한다.
    if (fLength <= FLT_EPSILON)
    {
        m_vPreviousPosition = vCurrent;
        return;
    }

    _vector vDirection = XMVector3Normalize(vMove);

    for (const SECTOR_LINK_DESC& Link : m_ChapterDesc.Links)
    {
        // 렌더 전용 링크는 섹터 전환 대상으로 사용하지 않는다.
        if (Link.eType == SECTOR_LINK_TYPE::RENDER_ONLY)
            continue;

        const _bool bForward = Link.SourceID == m_iCurrentSectorID;
        const _bool bBackward = Link.TwoWay && Link.TargetID == m_iCurrentSectorID;

        if (false == bForward && false == bBackward)
            continue;

        _vector vForward = Get_LinkWorldForward(Link);
        const _float fDot = XMVectorGetX(XMVector3Dot(vDirection, vForward));

        // 링크 진행 방향과 반대로 움직이는 경우에는 전환하지 않는다.
        if ((bForward && fDot <= 0.f) || (bBackward && fDot >= 0.f))
            continue;

        _float fDistance = 0.f;

        // 이전 위치에서 이동 방향으로 쏜 레이가 이번 프레임 이동 거리 안에서 링크를 지나면 전환한다.
        if (Intersects_RotatedLinkBounds(Link, XMLoadFloat3(&m_vPreviousPosition), vDirection, &fDistance) && fDistance <= fLength)
        {
            Change_Sector(Link);
            break;
        }
    }

    m_vPreviousPosition = vCurrent;
}

void CMapWorld::Change_Sector(const SECTOR_LINK_DESC& Link)
{
    _uint iTargetID = UINT_MAX;

    // 현재 섹터가 링크의 어느 쪽에 있는지 보고 반대편 섹터를 목표로 잡는다.
    if (m_iCurrentSectorID == Link.SourceID)
        iTargetID = Link.TargetID;
    else if (Link.TwoWay && m_iCurrentSectorID == Link.TargetID)
        iTargetID = Link.SourceID;

    if (UINT_MAX == iTargetID || m_LoadedMaps.end() == m_LoadedMaps.find(iTargetID))
        return;

    if (false == Set_CurrentSector(iTargetID))
        return;

    if (FAILED(Update_ActiveSectors()))
    {
        MSG_BOX("Active Render Sector Update Failed");
        return;
    }

    // 새 현재 섹터의 다음 이웃을 한 구간 앞서 준비해 최초 진입 생성을 방지한다.
    if (FAILED(Prepare_ActiveRenderResources()))
    {
        MSG_BOX("Active Render Resource Prewarm Failed");
        return;
    }

#ifdef _DEBUG
    Update_MapPhysicsDebugVisibility();
#endif

    // 섹터 변경 이벤트 (몬스터 소환)
    Update_ActiveSimulationSectors(true);

    SECTOR_CHANGED_EVENT_DESC EventDesc{};
    EventDesc.iPreviousSectorID = m_iPreviousSectorID;
    EventDesc.iCurrentSectorID = m_iCurrentSectorID;
    EventDesc.pActiveRenderSectorIDs = &m_ActiveRenderSectorIDs;
    EventDesc.pActiveSimulationSectorIDs = &m_ActiveSimulationSectorIDs;
    m_pGameInstance->Broadcast(EVENT_TYPE::SECTOR_CHANGED, &EventDesc);

    // 링크 볼륨을 빠져나갈 때까지 재전환을 막는다.
    m_bTransitionLocked = true;
    m_TransitionLockBounds = Link.Bounds;
}



CMapWorld* CMapWorld::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CHAPTER_DESC& ChapterDesc)
{
    CMapWorld* pInstance = new CMapWorld(pDevice, pContext);

    if (FAILED(pInstance->Initialize(ChapterDesc)))
    {
        Safe_Release(pInstance);
        return nullptr;
    }

    return pInstance;
}

void CMapWorld::Free()
{
#ifdef _DEBUG
    Reset_MapPhysicsDebugVisibility();
 #endif

    if (nullptr != m_pMapController)
        m_pMapController->Clear_Sectors();

    Safe_Release(m_pMapController);

    // CMap 인스턴스는 CMap_Manager가 소유하므로 여기서는 Release하지 않는다.
    m_RegisteredSectorIDs.clear();
    m_ActiveRenderSectorIDs.clear();
    m_ActiveSimulationSectorIDs.clear();
    m_ActiveRenderLinkIDs.clear();
    m_HLODRenderSectorIDs.clear();
    m_LoadedMaps.clear();
    m_LoadedHLODMaps.clear();
    m_ChapterDesc.Sectors.clear();
    m_ChapterDesc.Links.clear();

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}

#pragma once

#include "GameObject.h"
#include "Map.h"
#include "MapObject.h"

NS_BEGIN(Engine)
class CMapInstanceObject;
class CComputeShader;
class CMapRenderInstancePool;

class ENGINE_DLL CMapController final : public CGameObject
{
public:
    // 섹터 랜더링 방식
    enum class SECTOR_RENDER_MODE
    {
        FULL,
        PORTAL_FILTERED,
        HIDDEN
    };

    // 포탈 렌더시 사용할 포탈 영역/ 방향 / 깊이 정보 
    struct tSectorPortalRenderDesc
    {
        BoundingBox BoundingPortal = {};
        _float3 vRotation = {}; // Portal volume rotation in degrees.
        _float3 vDirection = {};
        _float fSidePadding = 0.5f;
        _float fViewDepth = 0.f;
        _bool bUseDepth = true;
    };

private:

    // 컬링 대상 오브젝트 or 인스턴스면 인덱스도 추가로 
    struct SPATIAL_ENTRY
    {
        CMapObject* pOwner = nullptr;
        _uint iInstanceIndex = UINT_MAX;
        _uint iGlobalInstanceIndex = UINT_MAX;
        BoundingBox WorldBounds{};
        mutable _uint iVisitStamp = 0;

        _bool Is_Instance() const { return UINT_MAX != iInstanceIndex; }
    };

    // octree 컬링용 노드 
    struct OCTREE_NODE
    {
        BoundingBox WorldBounds{};
        vector<_uint> EntryIndices;
        vector<_uint> SubtreeEntryIndices;

        OCTREE_NODE* Children[8] = {};
    };

    // xz 기준으로 나눈 청크 / 내부에 옥트리 
    struct MAP_CHUNK
    {
        _int iChunkCoordX = {};
        _int iChunkCoordZ = {};

        BoundingBox ChunkCellBounds{};
        BoundingBox WorldBounds{};

        vector<_uint> EntryIndices;
        OCTREE_NODE* pStaticOctree = nullptr;
    };

    // 섹터 하나의 맵 포인터, 공간 구조 , 랜더 정책을 모아둔 구조체
    struct SECTOR_SPATIAL_DATA
    {
        _uint iSectorID = {};
        CMap* pMap = nullptr;

        BoundingBox SpatialBounds{};
        _bool bHasSpatialBounds = false;

        SECTOR_RENDER_MODE eRenderMode = SECTOR_RENDER_MODE::HIDDEN;
        vector<tSectorPortalRenderDesc> PortalRenderDescs;
        vector<BoundingBox> PortalVolumes;

        vector<SPATIAL_ENTRY> SpatialEntries;
        vector<MAP_CHUNK> Chunks;
    };

private:
    CMapController(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CMapController() = default;

public:
    // 에디터용 
    _uint Get_TotalObjectCount() const { return m_iTotalObjectCount; }
    _uint Get_VisibleObjectCount() const { return m_iVisibleObjectCount; }

    ContainmentType Get_FrustumContainment(const BoundingBox& bounds, const BoundingFrustum& frustum) const;
    ContainmentType Get_DistanceContainment(const BoundingBox& bounds, _fvector vCameraPos) const;
    ContainmentType Get_PortalContainment(const BoundingBox& bounds, const vector<BoundingBox>* pPortalVolumes) const;

    _bool Is_BoundsVisible(const BoundingBox& bounds) const;

    void Set_SectorRenderFull(_uint iSectorID);
    void Set_SectorPortalRender(_uint iSectorID, const vector<tSectorPortalRenderDesc>& PortalRenderDescs);
    void Set_SectorRenderHidden(_uint iSectorID);

    _bool Set_SectorUseHLOD(_uint iSectorID, _bool bUseHLOD);

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void Late_Update(float fTimeDelta) override;

public:     // 섹터 등록 , 해제 , 클리어용 

    HRESULT Register_Sector(_uint iSectorID, CMap* pOriginalMap,CMap * pHLODMap);
    void    Unregister_Sector(_uint iSectorID);
    void    Clear_Sectors();
    HRESULT Prepare_GlobalRenderResources();
    HRESULT Prepare_SectorRenderResources(_uint iSectorID);

private:
    // 섹터 전체 공간 bounds, 청크, octree를 다시 만듬 
    void Build_SpatialStructure(SECTOR_SPATIAL_DATA& SectorData);
    HRESULT Prepare_MapRenderResources(CMap* pMap);

    // 맵 오브젝트 합쳐서 전체 바운드 박스 계산용
    void Calculate_SpatialBounds(SECTOR_SPATIAL_DATA& SectorData);

    void Update_TotalObjectCount(); // 에디터용 나중에 빼기

    // static + 인스턴스 객체를 공간엔트리로 풀고 청크에 넣음 
    void Rebuild_Chunks(SECTOR_SPATIAL_DATA& SectorData);

    // 컬링 대상 하나를 SpatialEntries에 추가하고 등록된 인덱스를 반환함
    _uint Add_SpatialEntry(SECTOR_SPATIAL_DATA& SectorData, CMapObject* pOwner, _uint iInstanceIndex, _uint iGlobalInstanceIndex, const BoundingBox& WorldBounds);

    // 오브젝트 bounds가 겹치는 모든 청크에 엔트리 인덱스를 등록함 
    void Insert_Object_ToOverlappedChunks(SECTOR_SPATIAL_DATA& SectorData, _uint iEntryIndex);

    // 청크마다 octree 등록
    void Build_Octrees(SECTOR_SPATIAL_DATA& SectorData);

    // 엔트리 목록을 받아 현재 bounds 안에 octree 노드 재귀 생성용
    OCTREE_NODE* Build_OctreeNode(const SECTOR_SPATIAL_DATA& SectorData, const BoundingBox& NodeBounds, vector<_uint> EntryIndices, _uint iDepth);

    // octree 해지 삭제 
    void Release_OctreeNode(OCTREE_NODE* pNode);
    void Clear_Chunks(SECTOR_SPATIAL_DATA& SectorData);

    // bounds가 완전히 들어가는 자식 노드 인덱스 찾는용
    _int Find_ContainingChildIndex(const BoundingBox& parentBounds, const BoundingBox& objectBounds) const;

    // 부모 bounds를 8분해서 자식 bounds 만드는 용
    BoundingBox Make_ChildBounds(const BoundingBox& parentBounds, _int iChildIndex) const;

    // octree 를 순회하면서 프리스텀 / 포탈 컬링 을 통과한 오브젝트 걸러네는 용도 
    void Traverse_Octree(const SECTOR_SPATIAL_DATA& SectorData, const OCTREE_NODE* pNode, const BoundingFrustum& frustum, _fvector vCameraPos, const vector<BoundingBox>* pPortalVolumes, ContainmentType eFrustumState, ContainmentType eDistanceState, ContainmentType ePortalState);
    // 보이는 인스턴스 행렬들을 인스턴스 버퍼에 넣고 랜더하는 용도
    void Flush_VisibleInstanceGroups();

    RENDERID Resolve_MapRenderGroup(CMaterial* pMaterial) const;

    // HLOD는 CPU 검사
    void Submit_HLODMap(CMap* pHLODMap, const SECTOR_SPATIAL_DATA& SectorPolicyData, const BoundingFrustum& frustum);


    // 같은 오브젝트가 한 프레임에 중복 제출되지 않게 랜더 그룹에 넣음 
    void Submit_Object_Once(CMapObject* pObject);

    void Emit_Entry_NoCull(const SPATIAL_ENTRY& Entry);

    void Add_VisibleInstanceIndex(CMapInstanceObject* pOwner, _uint iInstanceIndex);

    // 프리스텀
    BoundingFrustum Build_WorldFrustum() const;
    void Build_WorldFrustumPlanes(_float4 OutPlannes[6]) const ;
    MAP_CULL_CS_DESC Build_MapCullCSDesc(_int iCandidateCount) const;
    HRESULT Ready_MapInstanceCullCS();
    _bool Use_MapInstanceCSCull() const;


    // 포탈 검사용 
    _bool Is_BoundsVisible(const BoundingBox& bounds, const BoundingFrustum& frustum, _fvector vCameraPos) const;
    _bool Is_BoundsVisibleByPortal(const BoundingBox& bounds, const vector<BoundingBox>* pPortalVolumes) const;

    // 포탈 볼륨용
    BoundingBox Make_PortalVolume(const tSectorPortalRenderDesc& PortalDesc) const;
    _int Calc_ChunkCoord(_float fPosition) const;

    // 청크 좌표 기준으로 xz 평면 청크 bounds 만드는 용도
    BoundingBox Make_ChunkCellBounds(_int iChunkX, _int iChunkZ) const;

    // GPU-cull 후보 instance와 해당 owner를 같이 등록한다.
    void Add_GlobalCandidateIndex(CMapInstanceObject* pOwner, _uint iGlobalInstanceIndex);

private:
    unordered_map<_uint, SECTOR_SPATIAL_DATA> m_Sectors; // 등록된 섹터 데이터 보관함 

    unordered_map<_uint, CMap*> m_HLODMaps;

    unordered_set<_uint> m_HLODRenderSectorIDs;

    unordered_set<CMapObject*> m_SubmittedObjects; // 한 프레임에 이미 랜더 제출한 일반 오브젝트 기록용

    unordered_map<CMapInstanceObject*, vector<_uint>> m_VisibleInstanceIndices;
    vector<CMapInstanceObject*> m_FrameVisibleInstanceOwners;
    vector<CMapInstanceObject*> m_FrameGlobalCSOwners;
    unordered_set<CMapInstanceObject*> m_FrameGlobalCSOwnerSet;
    CMapRenderInstancePool* m_pMapRenderInstancePool = nullptr;

    BoundingFrustum m_FrameFrustum{};
    _float4 m_vFrameCameraPosition{};
    _bool m_bFrameVisibilityReady = false;

    _bool m_bFrameGlobalCSCullReady = false;

    // 에디터용
    _uint m_iTotalObjectCount = {};
    _uint m_iVisibleObjectCount = {};

    _uint m_iFrameStamp = { 0 };

    CMapInstanceObject* m_pCachedInstanceOwner = nullptr;
    vector<_uint>* m_pCachedInstanceIndices = nullptr;

    //청크 한칸 크기 / distance 컬링 거리 
    _float m_fChunkSize = { 64.f };
    _float m_fDefaultCullDistance = { 225.f };

    CComputeShader* m_pMapInstanceCullCS = nullptr;
    ID3D11Buffer* m_pMapCullCSConstantBuffer = nullptr;


    // octree leaf  분할 값 
    _uint m_iMaxObjectsPerLeaf = { 24 };
    _uint m_iMaxDepth = { 4 };
    _float m_fMinNodeSize = { 8.f };

public:
    static CMapController* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END

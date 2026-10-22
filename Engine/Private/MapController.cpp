#include "MapController.h"
#include "GameInstance.h"
#include "MapInstanceObject.h"
#include "MapRenderInstancePool.h"
#include "ComputeShader.h"

CMapController::CMapController(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CGameObject(pDevice,pContext)
{

}




HRESULT CMapController::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(nullptr)))
		return E_FAIL;

	m_pMapRenderInstancePool = CMapRenderInstancePool::Create(m_pDevice, m_pContext);

	if (nullptr == m_pMapRenderInstancePool)
		return E_FAIL;

	if (FAILED(Ready_MapInstanceCullCS()))
	{
		Safe_Release(m_pMapCullCSConstantBuffer);
		Safe_Release(m_pMapInstanceCullCS);
	}
	return S_OK;
}

void CMapController::Late_Update(float fTimeDelta)
{
	
	__super::Late_Update(fTimeDelta);

	// 프레임마다 새로 보이는 대상만 모아야 해서 이전 프레임 캐시 비움
	m_SubmittedObjects.clear();


	for(CMapInstanceObject* pOwner : m_FrameVisibleInstanceOwners)
	{
		if (nullptr == pOwner)
			continue;

		auto Iter = m_VisibleInstanceIndices.find(pOwner);

		if (m_VisibleInstanceIndices.end() == Iter)
			continue;

		Iter->second.clear();
	}

	m_FrameVisibleInstanceOwners.clear();
	m_FrameGlobalCSOwners.clear();
	m_FrameGlobalCSOwnerSet.clear();
	m_pCachedInstanceOwner = nullptr;
	m_pCachedInstanceIndices = nullptr;

	m_bFrameGlobalCSCullReady = false;

	if (nullptr != m_pMapRenderInstancePool)
		m_pMapRenderInstancePool->Begin_Frame();

	m_iVisibleObjectCount = 0;

	++m_iFrameStamp;


	const _float4* pCameraPosition = m_pGameInstance->Get_CamPosition();

	if (nullptr == pCameraPosition)
	{
		m_bFrameVisibilityReady = false;
		return;
	}

	m_FrameFrustum = Build_WorldFrustum();
	m_vFrameCameraPosition = *pCameraPosition;
	m_bFrameVisibilityReady = true;
	
	const BoundingFrustum& frustum = m_FrameFrustum;
	_vector vCameraPos = XMLoadFloat4(&m_vFrameCameraPosition);

	unordered_map<_uint, SECTOR_SPATIAL_DATA>::iterator Iter;

	for (Iter = m_Sectors.begin(); Iter != m_Sectors.end(); ++Iter)
	{
		SECTOR_SPATIAL_DATA& SectorData = Iter->second;


		if (nullptr == SectorData.pMap)
			continue;

		if (m_HLODRenderSectorIDs.end() != m_HLODRenderSectorIDs.find(Iter->first))
		{
			auto HLODIter = m_HLODMaps.find(Iter->first);

			if (m_HLODMaps.end() != HLODIter && nullptr != HLODIter->second)
			{
				// HLOD는 Original과 같은 FULL/PORTAL/HIDDEN 정책을 사용한다.
				Submit_HLODMap(HLODIter->second, SectorData, frustum);
				continue;
			}
		}


		// 맵 오브젝트 구성이 바뀐 섹터는 공간 구조 다시 만들어야 함
		if (SectorData.pMap->Is_SpatialDirty())
		{
			// bounds/chunk/octree를 현재 맵 상태 기준으로 재구성함
			Build_SpatialStructure(SectorData);
			// 재구성 끝났으니 dirty 플래그 내려 중복 빌드 막음
			SectorData.pMap->Set_SpatialDirty(false);
			// 오브젝트 수 통계도 구조 변경에 맞춰 다시 계산함
			Update_TotalObjectCount();
		}

		// 활성 오브젝트가 없어 공간 bounds가 없으면 컬링할 대상도 없음
		if (false == SectorData.bHasSpatialBounds)
			continue;

		// MapWorld에서 숨김 처리한 섹터는 렌더 검사 자체를 생략함
		if (SectorData.eRenderMode == SECTOR_RENDER_MODE::HIDDEN)
			continue;

		// 섹터 전체 bounds 컬링
		if (false == Is_BoundsVisible(SectorData.SpatialBounds, frustum, vCameraPos))
			continue;

		const vector<BoundingBox>* pPortalVolumes = nullptr;

		if (SECTOR_RENDER_MODE::PORTAL_FILTERED == SectorData.eRenderMode)
		{
			// Set_SectorPortalRender에서 만들어 둔 캐시를 그대로 사용한다.
			pPortalVolumes = &SectorData.PortalVolumes;
		}

		// 섹터 내부는 청크 단위로 한 번 더 거른 뒤 Octree를 탐색함
		for (size_t i = 0; i < SectorData.Chunks.size(); i++)
		{
			const MAP_CHUNK& Chunk = SectorData.Chunks[i];

			// 고정된 XZ 청크 셀이 포털 영역과 전혀 겹치지 않으면 제외한다.
			if (DISJOINT == Get_PortalContainment(Chunk.ChunkCellBounds, pPortalVolumes))
				continue;

			// 청크 전체 bounds와 프러스텀의 포함 관계를 구한다.
			ContainmentType eChunkFrustumState = Get_FrustumContainment(Chunk.WorldBounds, frustum);

			if (DISJOINT == eChunkFrustumState)
				continue;

			// 청크 전체 bounds와 거리 컬링 영역의 포함 관계를 구한다.
			ContainmentType eChunkDistanceState = Get_DistanceContainment(Chunk.WorldBounds, vCameraPos);

			if (DISJOINT == eChunkDistanceState)
				continue;

			// 옥트리 루트와 동일한 WorldBounds를 기준으로 포털 포함 관계를 구한다.
			ContainmentType eChunkPortalState = Get_PortalContainment(Chunk.WorldBounds, pPortalVolumes);

			if (DISJOINT == eChunkPortalState)
				continue;

			// 청크가 조건에 완전히 포함된 경우 해당 상태를 옥트리 루트에 전달한다.
			Traverse_Octree(SectorData, Chunk.pStaticOctree, frustum, vCameraPos, pPortalVolumes, eChunkFrustumState, eChunkDistanceState, eChunkPortalState);
		}
		
	}

	if (Use_MapInstanceCSCull() && nullptr != m_pMapRenderInstancePool)
	{
		const _uint iGlobalCandidateCount = m_pMapRenderInstancePool->Get_CandidateCount();

		if (0 != iGlobalCandidateCount)
		{
			MAP_CULL_CS_DESC GlobalCullDesc = Build_MapCullCSDesc(static_cast<_int>(iGlobalCandidateCount));
			HIZ_HISTORY_DESC HiZHistory{};
			ID3D11ShaderResourceView* pGlobalCullHiZSRV = m_pGameInstance->Get_HiZSRV();

			GlobalCullDesc.iUseHiZ = 0;

			// 첫 프레임에는 이전 프레임 Hi-Z가 없으므로 frustum/distance만 사용한다.
			if (nullptr != pGlobalCullHiZSRV && m_pGameInstance->Get_HiZHistory(&HiZHistory))
			{
				// Renderer가 Revision, 위치, 방향, Projection까지 검증한 History만 반환한다.
				GlobalCullDesc.fDepthBias = 0.002f;
				GlobalCullDesc.fScreenMargin = 0.002f;
				GlobalCullDesc.PreviousViewProjection = HiZHistory.PreviousViewProjection;
				GlobalCullDesc.vHiZTextureSize = HiZHistory.vTextureSize;
				GlobalCullDesc.iHiZMipCount = HiZHistory.iMipCount;
				GlobalCullDesc.iUseHiZ = 1;
		
			}
			else
			{
				pGlobalCullHiZSRV = nullptr;
				GlobalCullDesc.iUseHiZ = 0;
			}

			if (0 == GlobalCullDesc.iUseHiZ)
				pGlobalCullHiZSRV = nullptr;

			if (SUCCEEDED(m_pMapRenderInstancePool->Dispatch_GlobalCull_CS(GlobalCullDesc, m_pMapInstanceCullCS, m_pMapCullCSConstantBuffer, pGlobalCullHiZSRV)))
				m_bFrameGlobalCSCullReady = true;
		}
	}

	Flush_VisibleInstanceGroups();
}

HRESULT CMapController::Register_Sector(_uint iSectorID, CMap* pOriginalMap, CMap* pHLODMap)
{
	if (nullptr == pOriginalMap)
		return E_FAIL;

	// 같은 SectorID의 Original 또는 HLOD 중복 등록을 방지한다.
	if (m_Sectors.end() != m_Sectors.find(iSectorID) || m_HLODMaps.end() != m_HLODMaps.find(iSectorID))
		return E_FAIL;

	pair<unordered_map<_uint, SECTOR_SPATIAL_DATA>::iterator, bool> Result = m_Sectors.emplace(iSectorID, SECTOR_SPATIAL_DATA());

	if (false == Result.second)
		return E_FAIL;

	SECTOR_SPATIAL_DATA& SectorData = Result.first->second;
	SectorData.iSectorID = iSectorID;
	SectorData.pMap = pOriginalMap;

	// Original만 기존 공간 구조와 Instance Pool에 등록한다.
	Build_SpatialStructure(SectorData);
	pOriginalMap->Set_SpatialDirty(false);

	// HLOD는 공간 구조를 만들지 않고 렌더용 포인터만 보관한다.
	if (nullptr != pHLODMap)
		m_HLODMaps.emplace(iSectorID, pHLODMap);

	// 전체 오브젝트 통계는 기존과 동일하게 Original 기준으로 계산한다.
	Update_TotalObjectCount();

	return S_OK;
}

void CMapController::Unregister_Sector(_uint iSectorID)
{
	unordered_map<_uint, SECTOR_SPATIAL_DATA>::iterator Iter = m_Sectors.find(iSectorID);

	if (m_Sectors.end() == Iter)
		return;

	// Original이 가진 청크와 Octree 메모리를 정리한다.
	Clear_Chunks(Iter->second);
	m_Sectors.erase(Iter);

	// HLOD는 MapManager 소유 포인터이므로 컨테이너에서만 제거한다.
	m_HLODMaps.erase(iSectorID);
	m_HLODRenderSectorIDs.erase(iSectorID);

	m_VisibleInstanceIndices.clear();
	m_FrameVisibleInstanceOwners.clear();
	m_FrameGlobalCSOwners.clear();
	m_FrameGlobalCSOwnerSet.clear();
	m_pCachedInstanceOwner = nullptr;
	m_pCachedInstanceIndices = nullptr;

	Update_TotalObjectCount();
}

void CMapController::Clear_Sectors()
{
	unordered_map<_uint, SECTOR_SPATIAL_DATA>::iterator Iter;

	for (Iter = m_Sectors.begin(); Iter != m_Sectors.end(); ++Iter)
	{
		Clear_Chunks(Iter->second);
	}

	m_Sectors.clear();

	// HLOD는 MapManager 소유 포인터이므로 포인터와 선택 상태만 제거한다.
	m_HLODMaps.clear();
	m_HLODRenderSectorIDs.clear();

	m_iTotalObjectCount = 0;

	m_VisibleInstanceIndices.clear();
	m_FrameVisibleInstanceOwners.clear();
	m_pCachedInstanceOwner = nullptr;
	m_pCachedInstanceIndices = nullptr;

	if (nullptr != m_pMapRenderInstancePool)
		m_pMapRenderInstancePool->Clear();
}

HRESULT CMapController::Prepare_GlobalRenderResources()
{
    if (nullptr == m_pMapRenderInstancePool)
        return E_FAIL;

    // 첫 활성 섹터에서 만들던 공용 컬링/간접 드로우 버퍼를 로딩 단계로 이동한다.
    return m_pMapRenderInstancePool->Prepare_GPUResources();
}

HRESULT CMapController::Prepare_SectorRenderResources(_uint iSectorID)
{
    auto SectorIter = m_Sectors.find(iSectorID);

    if (m_Sectors.end() == SectorIter)
        return E_FAIL;

    if (FAILED(Prepare_MapRenderResources(SectorIter->second.pMap)))
        return E_FAIL;

    auto HLODIter = m_HLODMaps.find(iSectorID);

    if (m_HLODMaps.end() != HLODIter && FAILED(Prepare_MapRenderResources(HLODIter->second)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMapController::Prepare_MapRenderResources(CMap* pMap)
{
    if (nullptr == pMap)
        return S_OK;

    for (CMapObject* pObject : pMap->Get_Objects())
    {
        CMapInstanceObject* pInstanceObject = dynamic_cast<CMapInstanceObject*>(pObject);

        if (nullptr == pInstanceObject)
            continue;

        if (FAILED(pInstanceObject->Prepare_RenderResources()))
            return E_FAIL;
    }

    return S_OK;
}


void CMapController::Build_SpatialStructure(SECTOR_SPATIAL_DATA& SectorData)
{
	// 기존 청크/Octree가 남아 있으면 새 구조와 섞이므로 먼저 비움
	Clear_Chunks(SectorData);
	// 섹터 전체 bounds를 계산해 상위 컬링 기준을 만듦
	Calculate_SpatialBounds(SectorData);
	// 오브젝트들을 청크 단위로 분류함
	Rebuild_Chunks(SectorData);
	// 각 청크 내부를 Octree로 세분화함
	Build_Octrees(SectorData);
}

void CMapController::Calculate_SpatialBounds(SECTOR_SPATIAL_DATA& SectorData)
{
	SectorData.SpatialBounds = BoundingBox();
	SectorData.bHasSpatialBounds = false;

	if (nullptr == SectorData.pMap)
		return;

	// 섹터 맵이 가진 모든 오브젝트를 기준으로 전체 bounds를 만듦
	const vector<CMapObject*>& Objects = SectorData.pMap->Get_Objects();

	for (size_t i = 0; i < Objects.size(); ++i)
	{
		CMapObject* pObject = Objects[i];

		// 비활성/빈 오브젝트는 렌더 컬링 대상에서 제외함
		if (nullptr == pObject || pObject->Is_InActive())
			continue;

		// 오브젝트의 월드 AABB를 섹터 bounds에 합산함
		BoundingBox ObjectBounds = pObject->Get_WorldBounds();

		// 첫 유효 오브젝트는 merge 기준 bounds로 그대로 사용함
		if (false == SectorData.bHasSpatialBounds)
		{
			SectorData.SpatialBounds = ObjectBounds;
			SectorData.bHasSpatialBounds = true;
		}
		else
		{
			// 오브젝트는 기존 섹터 bounds와 병합해 전체 범위를 넓힘
			BoundingBox::CreateMerged(SectorData.SpatialBounds, SectorData.SpatialBounds, ObjectBounds);
		}
	}

}

void CMapController::Update_TotalObjectCount()
{
	m_iTotalObjectCount = 0;

	unordered_map<_uint, SECTOR_SPATIAL_DATA>::const_iterator SectorIter;

	for (SectorIter = m_Sectors.begin(); SectorIter != m_Sectors.end(); ++SectorIter)
	{
		CMap* pMap = SectorIter->second.pMap;

		if (nullptr == pMap)
			continue;

		// 맵 안 활성 오브젝트를 순회하며 실제 렌더 단위 수를 셈
		const vector<CMapObject*>& Objects =pMap->Get_Objects();

		for (size_t i = 0; i < Objects.size(); ++i)
		{
			CMapObject* pObject = Objects[i];

			// 비활성 오브젝트는 총 개수 통계에서도 제외함
			if (nullptr == pObject || pObject->Is_InActive())
				continue;

			// 인스턴스 오브젝트는 내부 인스턴스 개수만큼 렌더 대상이 늘어남
			CMapInstanceObject* pInstanceObject = dynamic_cast<CMapInstanceObject*>(pObject);

			if (nullptr != pInstanceObject)
			{
				m_iTotalObjectCount += pInstanceObject->Get_InstanceCount();
			}
			else
			{
				// 일반 오브젝트는 오브젝트 하나를 렌더 대상 하나로 봄
				++m_iTotalObjectCount;
			}
		}

	}

}

void CMapController::Rebuild_Chunks(SECTOR_SPATIAL_DATA& SectorData)
{
	if (nullptr == SectorData.pMap)
		return;

	if (nullptr != m_pMapRenderInstancePool)
		m_pMapRenderInstancePool->Release_SectorInstances(SectorData.iSectorID);

	// 맵의 오브젝트 목록을 SpatialEntry로 변환하기 위해 가져옴
	const vector<CMapObject*>& Objects = SectorData.pMap->Get_Objects();

	for (CMapObject* pObject : Objects)
	{
		if (nullptr == pObject || pObject->Is_InActive())
			continue;

		// 인스턴스 오브젝트인지 확인해서 일반/인스턴스 처리 분기함
		CMapInstanceObject* pInstanceOwner =dynamic_cast<CMapInstanceObject*>(pObject);

		if (nullptr == pInstanceOwner)
		{
			// 일반 오브젝트는 자기 bounds 하나로 엔트리 하나를 생성함
			const _uint iEntryIndex = Add_SpatialEntry(SectorData, pObject, UINT_MAX, UINT_MAX, pObject->Get_WorldBounds());

			// 해당 엔트리를 겹치는 모든 청크에 등록함
			Insert_Object_ToOverlappedChunks(SectorData, iEntryIndex);
			continue;
		}

		// 공간 구조를 만들기 전에 인스턴스 렌더 캐시의 월드 바운드를 갱신한다.
		if (FAILED(pInstanceOwner->Rebuild_RenderInstanceCache()))
			continue;

		for (_uint i = 0; i < pInstanceOwner->Get_InstanceCount(); i++)
		{
			const CMapInstanceObject::RENDER_INSTANCE_CACHE* pCache = pInstanceOwner->Get_RenderInstanceCache(i);

			if (nullptr == pCache || false == pCache->bRenderable)
				continue;

			const _bool bUseGPUCull = pInstanceOwner->Use_IndexedVisiblePath();

			_uint iGlobalInstanceIndex = UINT_MAX;

			if (nullptr != m_pMapRenderInstancePool)
			{
				iGlobalInstanceIndex = m_pMapRenderInstancePool->Register_Instance(SectorData.iSectorID, pInstanceOwner, i, pCache->WorldMatrix, pCache->WorldBounds, pCache->bRenderable, bUseGPUCull);
			}

			const _uint iEntryIndex = Add_SpatialEntry(SectorData, pInstanceOwner, i, iGlobalInstanceIndex, pCache->WorldBounds);

			Insert_Object_ToOverlappedChunks(SectorData, iEntryIndex);
		}
	
	}
}

_uint CMapController::Add_SpatialEntry(SECTOR_SPATIAL_DATA& SectorData, CMapObject* pOwner, _uint iInstanceIndex, _uint iGlobalInstanceIndex, const BoundingBox& WorldBounds)
{
	SPATIAL_ENTRY Entry{};
	Entry.pOwner = pOwner;
	Entry.iInstanceIndex = iInstanceIndex;
	Entry.iGlobalInstanceIndex = iGlobalInstanceIndex;
	Entry.WorldBounds = WorldBounds;

	const _uint iEntryIndex = static_cast<_uint>(SectorData.SpatialEntries.size());

	SectorData.SpatialEntries.push_back(Entry);
	return iEntryIndex;
}

void CMapController::Insert_Object_ToOverlappedChunks(SECTOR_SPATIAL_DATA& SectorData, _uint iEntryIndex)
{
	if (iEntryIndex>=SectorData.SpatialEntries.size())
		return;

	// 오브젝트의 월드 AABB를 기준으로 어느 Chunk들과 겹치는지 계산한다.
	const BoundingBox& Bounds = SectorData.SpatialEntries[iEntryIndex].WorldBounds;

	// AABB의 최소/최대 XZ를 청크 좌표로 변환해 겹치는 범위를 구함
	const _int iMinX = Calc_ChunkCoord(Bounds.Center.x - Bounds.Extents.x);
	const _int iMaxX = Calc_ChunkCoord(Bounds.Center.x + Bounds.Extents.x);
	const _int iMinZ = Calc_ChunkCoord(Bounds.Center.z - Bounds.Extents.z);
	const _int iMaxZ = Calc_ChunkCoord(Bounds.Center.z + Bounds.Extents.z);

	const long long iChunkCountX = static_cast<long long>(iMaxX) - static_cast<long long>(iMinX) + 1;
	const long long iChunkCountZ = static_cast<long long>(iMaxZ) - static_cast<long long>(iMinZ) + 1;
	const long long iOverlappedChunkCount = iChunkCountX * iChunkCountZ;
	const long long iMaxChunkRegisterCount = 1024;

	if (iOverlappedChunkCount <= 0)
		return;

	if (iOverlappedChunkCount > iMaxChunkRegisterCount)
	{
		// 큰 청크를 단일 특수 청크로 등록함.
		MAP_CHUNK Chunk{};
		Chunk.iChunkCoordX = 0x7fffffff;
		Chunk.iChunkCoordZ = 0x7fffffff;
		Chunk.ChunkCellBounds = Bounds;
		Chunk.WorldBounds = Bounds;
		Chunk.EntryIndices.push_back(iEntryIndex);
		SectorData.Chunks.push_back(Chunk);
		return;
	}


	// 오브젝트가 걸친 모든 청크 좌표를 순회함
	for (_int z = iMinZ; z <= iMaxZ; ++z)
	{
		for (_int x = iMinX; x <= iMaxX; ++x)
		{
			// 이미 만들어진 청크인지 좌표로 찾음
			auto Iter = find_if(
				SectorData.Chunks.begin(), SectorData.Chunks.end(),
				[x, z](const MAP_CHUNK& Chunk)
				{
					return Chunk.iChunkCoordX == x &&
						Chunk.iChunkCoordZ == z;
				});

			if (Iter == SectorData.Chunks.end())
			{
				// 아직 없는 청크면 셀 bounds와 첫 엔트리를 넣어 새로 생성함
				MAP_CHUNK Chunk{};
				Chunk.iChunkCoordX = x;
				Chunk.iChunkCoordZ = z;
				Chunk.ChunkCellBounds = Make_ChunkCellBounds(x, z);
				Chunk.WorldBounds = Bounds;
				Chunk.EntryIndices.push_back(iEntryIndex);
				SectorData.Chunks.push_back(Chunk);
			}
			else
			{
				// 기존 청크면 엔트리만 추가하고 청크 월드 bounds를 넓힘
				Iter->EntryIndices.push_back(iEntryIndex);
				BoundingBox::CreateMerged(
					Iter->WorldBounds, Iter->WorldBounds, Bounds);
			}
		}
	}
	
}

void CMapController::Build_Octrees(SECTOR_SPATIAL_DATA& SectorData)
{
	// 청크마다 독립 Octree를 만들어 청크 내부 탐색 비용을 줄임
	for (size_t i = 0; i < SectorData.Chunks.size(); i++)
	{
		MAP_CHUNK& Chunk = SectorData.Chunks[i];

		// 재빌드 상황에서 기존 트리가 남아 있으면 먼저 해제함
		Release_OctreeNode(Chunk.pStaticOctree);

		// 청크 bounds와 청크에 들어간 엔트리 목록으로 루트 노드 생성함
		Chunk.pStaticOctree = Build_OctreeNode(SectorData, Chunk.WorldBounds, std::move(Chunk.EntryIndices), 0);
	}
}


CMapController::OCTREE_NODE* CMapController::Build_OctreeNode(const SECTOR_SPATIAL_DATA& SectorData, const BoundingBox& NodeBounds, vector<_uint> EntryIndices, _uint iDepth)
{
	OCTREE_NODE* pNode = new OCTREE_NODE{};
	pNode->WorldBounds = NodeBounds;

	const _float fNodeSize = max(max(NodeBounds.Extents.x, NodeBounds.Extents.y), NodeBounds.Extents.z) * 2.f;
	const _bool bCanSplit = EntryIndices.size() > m_iMaxObjectsPerLeaf && iDepth < m_iMaxDepth && fNodeSize > m_fMinNodeSize;

	if (false == bCanSplit)
	{
		pNode->EntryIndices = std::move(EntryIndices);
		return pNode;
	}

	vector<_uint> ChildEntries[8];

	for (_uint iEntryIndex : EntryIndices)
	{
		if (iEntryIndex >= SectorData.SpatialEntries.size())
			continue;

		const SPATIAL_ENTRY& Entry = SectorData.SpatialEntries[iEntryIndex];
		const _int iChildIndex = Find_ContainingChildIndex(NodeBounds, Entry.WorldBounds);

		if (-1 == iChildIndex)
			pNode->EntryIndices.push_back(iEntryIndex);
		else
			ChildEntries[iChildIndex].push_back(iEntryIndex);
	}

	for (_int i = 0; i < 8; ++i)
	{
		if (ChildEntries[i].empty())
			continue;

		pNode->Children[i] = Build_OctreeNode(SectorData, Make_ChildBounds(NodeBounds, i), std::move(ChildEntries[i]), iDepth + 1);
	}

	pNode->SubtreeEntryIndices = pNode->EntryIndices;
	for (_int i = 0; i < 8; ++i)
	{
		const OCTREE_NODE* pChild = pNode->Children[i];
		if (nullptr == pChild)
			continue;

		const vector<_uint>& ChildSubTree = pChild->SubtreeEntryIndices.empty() ? pChild->EntryIndices : pChild->SubtreeEntryIndices;
		pNode->SubtreeEntryIndices.insert(pNode->SubtreeEntryIndices.end(), ChildSubTree.begin(), ChildSubTree.end());
	}

	return pNode;
}

void CMapController::Release_OctreeNode(OCTREE_NODE* pNode)
{
	if (nullptr == pNode)
		return;

	// 자식 노드부터 재귀 해제해야 하위 메모리가 남지 않음
	for (_uint i = 0; i < 8; ++i)
	{
		Release_OctreeNode(pNode->Children[i]);
		pNode->Children[i] = nullptr;
	}

	// 모든 자식 정리 후 현재 노드 해제함
	delete pNode;
}

void CMapController::Clear_Chunks(SECTOR_SPATIAL_DATA& SectorData)
{
	// 청크마다 들고 있는 Octree 루트를 해제함
	for (size_t i = 0; i < SectorData.Chunks.size(); ++i)
	{
		MAP_CHUNK& Chunk = SectorData.Chunks[i];

		Release_OctreeNode(Chunk.pStaticOctree);
		Chunk.pStaticOctree = nullptr;
	}
	// 청크와 엔트리 목록을 비워 다음 빌드가 새 상태에서 시작하게 함
	SectorData.Chunks.clear();
	SectorData.SpatialEntries.clear();
	SectorData.bHasSpatialBounds = false;
}

_int CMapController::Find_ContainingChildIndex(const BoundingBox& parentBounds, const BoundingBox& objectBounds) const
{
	_int iChildIndex = 0;

	if (objectBounds.Center.x > parentBounds.Center.x)
		iChildIndex |= 1;

	if (objectBounds.Center.y > parentBounds.Center.y)
		iChildIndex |= 2;

	if (objectBounds.Center.z > parentBounds.Center.z)
		iChildIndex |= 4;

	const BoundingBox ChildBounds = Make_ChildBounds(parentBounds, iChildIndex);

	if (CONTAINS != ChildBounds.Contains(objectBounds))
		return -1;

	return iChildIndex;
}

BoundingBox CMapController::Make_ChildBounds(const BoundingBox& parentBounds, _int iChildIndex) const
{
	BoundingBox childBounds{};

	// child는 parent의 절반 크기를 가진다.
	childBounds.Extents = {
		parentBounds.Extents.x * 0.5f,
		parentBounds.Extents.y * 0.5f,
		parentBounds.Extents.z * 0.5f
	};

	// iChildIndex의 bit를 이용해서 8방향 위치를 결정한다.
	// bit 0: x 방향
	// bit 1: y 방향
	// bit 2: z 방향
	childBounds.Center = parentBounds.Center;
	childBounds.Center.x += (iChildIndex & 1) ? childBounds.Extents.x : -childBounds.Extents.x;
	childBounds.Center.y += (iChildIndex & 2) ? childBounds.Extents.y : -childBounds.Extents.y;
	childBounds.Center.z += (iChildIndex & 4) ? childBounds.Extents.z : -childBounds.Extents.z;

	return childBounds;
}

void CMapController::Traverse_Octree(const SECTOR_SPATIAL_DATA& SectorData, const OCTREE_NODE* pNode, const BoundingFrustum& frustum, _fvector vCameraPos, const vector<BoundingBox>* pPortalVolumes, ContainmentType eFrustumState, ContainmentType eDistanceState, ContainmentType ePortalState) 
{
	
	if (nullptr == pNode || DISJOINT == eFrustumState || DISJOINT == eDistanceState || DISJOINT == ePortalState)
		return;

	const _bool bNodeFrustumContained = CONTAINS == eFrustumState;
	const _bool bNodeDistanceContained = CONTAINS == eDistanceState;
	const _bool bNodePortalContained = CONTAINS == ePortalState;

	if (bNodeFrustumContained && bNodeDistanceContained && bNodePortalContained)
	{
		const vector<_uint>& AllEntries = pNode->SubtreeEntryIndices.empty() ? pNode->EntryIndices : pNode->SubtreeEntryIndices;

		for (_uint iEntryIndex : AllEntries)
		{
			if (iEntryIndex >= SectorData.SpatialEntries.size())
				continue;

			const SPATIAL_ENTRY& Entry = SectorData.SpatialEntries[iEntryIndex];

			if (Entry.iVisitStamp == m_iFrameStamp)
				continue;
			Entry.iVisitStamp = m_iFrameStamp;

			Emit_Entry_NoCull(Entry);
		}
		return;
	}


	// 현재 노드가 직접 보관하고 있는 엔트리를 검사한다.
	for (_uint iEntryIndex : pNode->EntryIndices)
	{
		// 잘못된 엔트리 인덱스는 처리하지 않는다.
		if (iEntryIndex >= SectorData.SpatialEntries.size())
			continue;

		const SPATIAL_ENTRY& Entry = SectorData.SpatialEntries[iEntryIndex];

		if (Entry.iVisitStamp == m_iFrameStamp)
			continue;
		Entry.iVisitStamp = m_iFrameStamp;

		// 일반 맵 오브젝트 처리
		if (false == Entry.Is_Instance())
		{
			CMapObject* pObject = Entry.pOwner;

			// 소유자가 없거나 비활성 상태면 렌더하지 않는다.
			if (nullptr == pObject || pObject->Is_InActive())
				continue;

			// 이미 렌더 그룹에 제출한 오브젝트면 중복 처리하지 않는다.
			if (m_SubmittedObjects.find(pObject) != m_SubmittedObjects.end())
				continue;

			// 노드가 거리 범위에 완전히 포함되지 않았을 때만 엔트리 거리 검사를 수행한다.
			if (false == bNodeDistanceContained && DISJOINT == Get_DistanceContainment(Entry.WorldBounds, vCameraPos))
				continue;

			// 노드가 포털에 완전히 포함되지 않았을 때만 엔트리 포털 검사를 수행한다.
			if (false == bNodePortalContained && DISJOINT == Get_PortalContainment(Entry.WorldBounds, pPortalVolumes))
				continue;

			// 노드가 프러스텀에 완전히 포함되지 않았을 때만 엔트리 프러스텀 검사를 수행한다.
			if (false == bNodeFrustumContained && DISJOINT == Get_FrustumContainment(Entry.WorldBounds, frustum))
				continue;

			++m_iVisibleObjectCount;
			Submit_Object_Once(pObject);
			continue;
		}

		// 인스턴스 엔트리의 실제 소유자를 가져온다.
		CMapInstanceObject* pOwner = static_cast<CMapInstanceObject*>(Entry.pOwner);

		// 인스턴스 소유자가 없거나 비활성 상태면 처리하지 않는다.
		if (nullptr == pOwner || pOwner->Is_InActive())
			continue;


		const CMapInstanceObject::RENDER_INSTANCE_CACHE* pCache = pOwner->Get_RenderInstanceCache(Entry.iInstanceIndex);

		if (nullptr == pCache || false == pCache->bRenderable)
			continue;

		const _bool bUseGlobalCSCandidate = Use_MapInstanceCSCull() && pOwner->Use_IndexedVisiblePath();

		// 포털은 방/문/섹터 정책에 가까워서 CPU에서 유지한다.
		if (false == bNodePortalContained && DISJOINT == Get_PortalContainment(Entry.WorldBounds, pPortalVolumes))
			continue;

		// GPU-cull 대상은 CPU fine culling / CPU visible upload로 보내지 않고 global candidate만 등록한다.
		if (bUseGlobalCSCandidate)
		{
			Add_GlobalCandidateIndex(pOwner, Entry.iGlobalInstanceIndex);
			continue;
		}

		// CPU 렌더 경로 대상만 distance/frustum fine culling 후 owner visible list에 넣는다.
		if (false == bNodeDistanceContained && DISJOINT == Get_DistanceContainment(Entry.WorldBounds, vCameraPos))
			continue;

		if (false == bNodeFrustumContained && DISJOINT == Get_FrustumContainment(Entry.WorldBounds, frustum))
			continue;

		Add_VisibleInstanceIndex(pOwner, Entry.iInstanceIndex);
	}

	// 현재 노드의 포함 상태를 자식 노드에 전달한다.
	for (_uint i = 0; i < 8; ++i)
	{
		const OCTREE_NODE* pChild = pNode->Children[i];

		if (nullptr == pChild)
			continue;

		const ContainmentType eChildFrustumState = bNodeFrustumContained ? CONTAINS : Get_FrustumContainment(pChild->WorldBounds, frustum);

		if (DISJOINT == eChildFrustumState)
			continue;

		const ContainmentType eChildDistanceState = bNodeDistanceContained ? CONTAINS : Get_DistanceContainment(pChild->WorldBounds, vCameraPos);

		if (DISJOINT == eChildDistanceState)
			continue;

		const ContainmentType eChildPortalState = bNodePortalContained ? CONTAINS : Get_PortalContainment(pChild->WorldBounds, pPortalVolumes);

		if (DISJOINT == eChildPortalState)
			continue;

		Traverse_Octree(SectorData, pChild, frustum, vCameraPos, pPortalVolumes, eChildFrustumState, eChildDistanceState, eChildPortalState);
	}
}

void CMapController::Flush_VisibleInstanceGroups()
{
	for (CMapInstanceObject* pOwner : m_FrameVisibleInstanceOwners)
	{
		if (nullptr == pOwner)
			continue;

		auto Iter = m_VisibleInstanceIndices.find(pOwner);

		if (m_VisibleInstanceIndices.end() == Iter)
			continue;

		vector<_uint>& VisibleIndices = Iter->second;

		if (FAILED(pOwner->Update_VisibleInstanceBuffer(VisibleIndices)))
			continue;

		const _uint iVisibleInstanceCount = static_cast<_uint>(VisibleIndices.size());
		m_iVisibleObjectCount += iVisibleInstanceCount;

		RENDERID eGroup = Resolve_MapRenderGroup(pOwner->Get_Material());

		if (eGroup == RENDERID::END)
			continue;

		m_pGameInstance->Add_RenderGroup(eGroup, pOwner);
	}

	// global CS 결과가 준비된 owner는 CPU visible upload 없이 indirect draw path로 제출한다.
	if (false == m_bFrameGlobalCSCullReady || nullptr == m_pMapRenderInstancePool)
		return;

	for (CMapInstanceObject* pOwner : m_FrameGlobalCSOwners)
	{
		if (nullptr == pOwner || pOwner->Is_InActive())
			continue;

		CMapRenderInstancePool::MAP_OWNER_GLOBAL_DRAW_DESC DrawDesc{};

		if (false == m_pMapRenderInstancePool->Get_OwnerGlobalDrawDesc(pOwner, &DrawDesc))
			continue;

		if (FAILED(pOwner->Set_GlobalIndirectRenderResources(DrawDesc.pGlobalWorldSRV, DrawDesc.pGroupVisibleIndexSRV, DrawDesc.pIndirectArgsBuffer, DrawDesc.iIndirectArgsOffset, DrawDesc.iVisibleBase)))
			continue;

		RENDERID eGroup = Resolve_MapRenderGroup(pOwner->Get_Material());

		if (eGroup == RENDERID::END)
			continue;

		m_pGameInstance->Add_RenderGroup(eGroup, pOwner);
	}
}

RENDERID CMapController::Resolve_MapRenderGroup(CMaterial* pMaterial) const
{
	if (nullptr == pMaterial)
		return RENDERID::NONBLEND;

	// 머테리얼이 결정한 렌더 그룹을 유지해 EYELASH가 FORWARD 패스로 진입하게 한다.
	return pMaterial->Get_RenderGroup();
}


void CMapController::Add_VisibleInstanceIndex(CMapInstanceObject* pOwner, _uint iInstanceIndex)
{
	if (nullptr == pOwner)
		return;

	if (pOwner != m_pCachedInstanceOwner)
	{
		m_pCachedInstanceOwner = pOwner;
		m_pCachedInstanceIndices = &m_VisibleInstanceIndices[pOwner];
	}

	if (m_pCachedInstanceIndices->empty())
		m_FrameVisibleInstanceOwners.push_back(pOwner);

	m_pCachedInstanceIndices->push_back(iInstanceIndex);
}

void CMapController::Emit_Entry_NoCull(const SPATIAL_ENTRY& Entry)
{
	if (false == Entry.Is_Instance())
	{
		CMapObject* pObject = Entry.pOwner;

		if (nullptr == pObject || pObject->Is_InActive())
			return;
	
		if (m_SubmittedObjects.find(pObject) != m_SubmittedObjects.end())
			return;

		++m_iVisibleObjectCount;
		Submit_Object_Once(pObject);
		return;
	}

	CMapInstanceObject* pOwner = static_cast<CMapInstanceObject*>(Entry.pOwner);

	if (nullptr == pOwner || pOwner->Is_InActive())
		return;

	const CMapInstanceObject::RENDER_INSTANCE_CACHE* pCache = pOwner->Get_RenderInstanceCache(Entry.iInstanceIndex);

	if (nullptr == pCache || false == pCache->bRenderable)
		return;

	// 완전 포함 노드에서도 GPU-cull 대상은 CPU visible upload로 보내지 않는다.
	if (Use_MapInstanceCSCull() && pOwner->Use_IndexedVisiblePath())
	{
		Add_GlobalCandidateIndex(pOwner, Entry.iGlobalInstanceIndex);
		return;
	}

	Add_VisibleInstanceIndex(pOwner, Entry.iInstanceIndex);
}

void CMapController::Submit_HLODMap(CMap* pHLODMap, const SECTOR_SPATIAL_DATA& SectorPolicyData, const BoundingFrustum& frustum)
{
	if (nullptr == pHLODMap)
		return;


	if (SECTOR_RENDER_MODE::HIDDEN == SectorPolicyData.eRenderMode)
		return;
	

	const vector<BoundingBox>* pPortalVolumes = nullptr;

	if (SECTOR_RENDER_MODE::PORTAL_FILTERED == SectorPolicyData.eRenderMode)
		pPortalVolumes = &SectorPolicyData.PortalVolumes;

	const vector<CMapObject*>& HLODObjects = pHLODMap->Get_Objects();

	for (CMapObject* pObject : HLODObjects)
	{
		if (nullptr == pObject || pObject->Is_InActive())
			continue;

		const BoundingBox WorldBounds = pObject->Get_WorldBounds();

		if (DISJOINT == Get_FrustumContainment(WorldBounds, frustum))
		{

			continue;
		}

		if (DISJOINT == Get_PortalContainment(WorldBounds, pPortalVolumes))
		{
			continue;
		}

		++m_iVisibleObjectCount;
		Submit_Object_Once(pObject);
	}

}

void CMapController::Submit_Object_Once(CMapObject* pObject)
{
	// 빈 오브젝트는 제출할 수 없음
	if (nullptr == pObject)
		return;

	// 같은 CMapObject가 여러 Chunk 또는 여러 탐색 경로에서 발견될 수 있으므로,
	// 이미 이번 프레임에 Submit한 오브젝트라면 다시 제출하지 않는다.
	if (m_SubmittedObjects.find(pObject) != m_SubmittedObjects.end())
		return;

	// 이번 프레임 제출 기록에 추가해 이후 중복 제출을 막음
	m_SubmittedObjects.emplace(pObject);

	// 최종적으로 보이는 오브젝트만 Renderer에 제출한다.
	// CMapObject 자체 Late_Update에서는 Add_RenderGroup을 하지 않는 구조여야 한다.

	RENDERID eGroup = Resolve_MapRenderGroup(pObject->Get_Material());
	if (eGroup == RENDERID::END)
		return;

	m_pGameInstance->Add_RenderGroup(eGroup, pObject);
}

BoundingFrustum CMapController::Build_WorldFrustum() const
{
	BoundingFrustum Frustum{};

	_matrix matProj = XMLoadFloat4x4(m_pGameInstance->Get_Transform(D3DTS::PROJ));
	_matrix matViewInv = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Inverse(D3DTS::VIEW));

	BoundingFrustum::CreateFromMatrix(Frustum, matProj);
	Frustum.Transform(Frustum, matViewInv);

	return Frustum;
}

void CMapController::Build_WorldFrustumPlanes(_float4 OutPlanes[6]) const
{
	_matrix matView = XMLoadFloat4x4(m_pGameInstance->Get_Transform(D3DTS::VIEW));
	_matrix matProj = XMLoadFloat4x4(m_pGameInstance->Get_Transform(D3DTS::PROJ));
	_matrix matViewProj = XMMatrixMultiply(matView, matProj);

	_matrix matViewProjT = XMMatrixTranspose(matViewProj);

	_vector vCol0 = matViewProjT.r[0];
	_vector vCol1 = matViewProjT.r[1];
	_vector vCol2 = matViewProjT.r[2];
	_vector vCol3 = matViewProjT.r[3];

	_vector Planes[6] =
	{
		XMPlaneNormalize(vCol3 + vCol0), // Left
		XMPlaneNormalize(vCol3 - vCol0), // Right
		XMPlaneNormalize(vCol3 + vCol1), // Bottom
		XMPlaneNormalize(vCol3 - vCol1), // Top
		XMPlaneNormalize(vCol2),         // Near
		XMPlaneNormalize(vCol3 - vCol2)  // Far
	};

	for (_uint i = 0; i < 6; ++i)
		XMStoreFloat4(&OutPlanes[i], Planes[i]);
}

MAP_CULL_CS_DESC CMapController::Build_MapCullCSDesc(_int iCandidateCount) const
{
	MAP_CULL_CS_DESC Desc{};

	Build_WorldFrustumPlanes(Desc.vFrustumPlanes);

	const _float4* pCameraPos = m_pGameInstance->Get_CamPosition();

	if (nullptr != pCameraPos)
		Desc.vCameraPosition = *pCameraPos;

	Desc.fCullDistance = m_fDefaultCullDistance;
	Desc.iCandidateCount = iCandidateCount;

	return Desc;
}

HRESULT CMapController::Ready_MapInstanceCullCS()
{
	const wstring strShaderDir = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/";

	CComputeShader::COMPUTE_SHADER_DESC Desc{};
	Desc.strShaderName = L"Shader_MapInstanceCullCS";
	Desc.strCSFilePath = strShaderDir + L"Shader_MapInstanceCullCS.hlsl";
	Desc.strCSFunName = "CS_Main";

	m_pMapInstanceCullCS = CComputeShader::Create(m_pDevice,m_pContext,Desc);

	if (nullptr == m_pMapInstanceCullCS)
		return E_FAIL;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.ByteWidth	= sizeof(MAP_CULL_CS_DESC);
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;

	if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pMapCullCSConstantBuffer)))
		return E_FAIL;

	return S_OK;
}

_bool CMapController::Use_MapInstanceCSCull() const
{
	return nullptr != m_pMapInstanceCullCS && nullptr != m_pMapCullCSConstantBuffer;
}



ContainmentType CMapController::Get_FrustumContainment(const BoundingBox& bounds, const BoundingFrustum& frustum) const
{
	// 프러스텀과 Bounds의 관계를 그대로 반환한다.
	return frustum.Contains(bounds);
}

ContainmentType CMapController::Get_DistanceContainment(const BoundingBox& bounds, _fvector vCameraPos) const
{
	const _vector vBoundsCenter = XMLoadFloat3(&bounds.Center);
	const _vector vCenterOffset = XMVectorSubtract(vBoundsCenter, vCameraPos);
	const _vector vBoundsExtents = XMLoadFloat3(&bounds.Extents);
	const _float fCenterDistanceSq = XMVectorGetX(XMVector3LengthSq(vCenterOffset));
	const _float fRadiusSq = XMVectorGetX(XMVector3LengthSq(vBoundsExtents));
	const _float fCullDistanceSq = m_fDefaultCullDistance * m_fDefaultCullDistance;
	const _float fBoundarySq = 4.f * fCullDistanceSq * fRadiusSq;
	const _float fDisjointValue = fCenterDistanceSq - fCullDistanceSq - fRadiusSq;

	if (fDisjointValue > 0.f && fDisjointValue * fDisjointValue > fBoundarySq)
		return DISJOINT;

	const _float fContainsValue = fCullDistanceSq + fRadiusSq - fCenterDistanceSq;

	if (fCullDistanceSq >= fRadiusSq && fContainsValue >= 0.f && fContainsValue * fContainsValue >= fBoundarySq)
		return CONTAINS;

	return INTERSECTS;
}

ContainmentType CMapController::Get_PortalContainment(const BoundingBox& bounds, const vector<BoundingBox>* pPortalVolumes) const
{
	// 포털 필터를 사용하지 않으면 모든 Bounds가 포함된 것으로 처리한다.
	if (nullptr == pPortalVolumes)
		return CONTAINS;

	// 포털 필터 상태에서 볼륨이 없으면 렌더 허용 영역이 없다.
	if (pPortalVolumes->empty())
		return DISJOINT;

	_bool bIntersects = false;

	for (const BoundingBox& PortalVolume : *pPortalVolumes)
	{
		ContainmentType eState = PortalVolume.Contains(bounds);

		// 하나의 포털 볼륨에 완전히 포함되면 하위 검사를 생략할 수 있다.
		if (CONTAINS == eState)
			return CONTAINS;

		// 하나 이상의 포털과 일부 겹치면 하위에서 계속 검사한다.
		if (INTERSECTS == eState)
			bIntersects = true;
	}

	return bIntersects ? INTERSECTS : DISJOINT;
}

_bool CMapController::Is_BoundsVisible(const BoundingBox& bounds) const
{
	if (false == m_bFrameVisibilityReady)
		return false;

	_vector vCameraPosition = XMLoadFloat4(&m_vFrameCameraPosition);

	// 저렴한 거리 검사를 먼저 통과시킨다.
	if (DISJOINT == Get_DistanceContainment(bounds, vCameraPosition))
		return false;

	// 거리 범위 안에 있는 Bounds만 Frustum 검사한다.
	return DISJOINT != Get_FrustumContainment(bounds, m_FrameFrustum);
}

_bool CMapController::Is_BoundsVisible(const BoundingBox& bounds, const BoundingFrustum& frustum, _fvector vCameraPos) const
{
	if (DISJOINT == Get_FrustumContainment(bounds, frustum))
		return false;

	return DISJOINT != Get_DistanceContainment(bounds, vCameraPos);
}

_bool CMapController::Is_BoundsVisibleByPortal(const BoundingBox& bounds, const vector<BoundingBox>* pPortalVolumes) const
{
	// 포탈 볼륨 포인터가 없으면 포탈 필터를 쓰지 않는 상태라 전체 허용함
	if (nullptr == pPortalVolumes)
		return true;

	// 포탈 필터 상태인데 볼륨이 하나도 없으면 아무것도 보이면 안 됨
	if (pPortalVolumes->empty())
		return false;

	// 하나라도 포탈 볼륨과 겹치면 렌더 허용 영역으로 봄
	for (const BoundingBox& PortalVolume : *pPortalVolumes)
	{
		if (PortalVolume.Intersects(bounds))
			return true;
	}

	return false;
}


BoundingBox CMapController::Make_PortalVolume(const tSectorPortalRenderDesc& PortalDesc) const
{

    BoundingBox LocalVolume{};
    LocalVolume.Center = _float3(0.f, 0.f, 0.f);
    LocalVolume.Extents = PortalDesc.BoundingPortal.Extents;

    _matrix matRotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(PortalDesc.vRotation.x), XMConvertToRadians(PortalDesc.vRotation.y), XMConvertToRadians(PortalDesc.vRotation.z));
    _matrix matInvRotation = XMMatrixTranspose(matRotation);

    if (PortalDesc.bUseDepth)
    {
        // Expand the portal in link-local space, then wrap the rotated result as a world AABB.
        _vector vDirection = XMLoadFloat3(&PortalDesc.vDirection);

        if (XMVectorGetX(XMVector3LengthSq(vDirection)) <= FLT_EPSILON)
            vDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);

        vDirection = XMVector3Normalize(XMVector3TransformNormal(vDirection, matInvRotation));

        _float3 vDirectionValue{};
        XMStoreFloat3(&vDirectionValue, vDirection);

        const _float fBasePadding = 0.5f;
        const _float fSidePadding = max(fBasePadding, PortalDesc.fSidePadding);
        const _float fDepth = PortalDesc.fViewDepth > 0.f ? PortalDesc.fViewDepth : m_fChunkSize * 0.5f;
        const _float fHalfDepth = fDepth * 0.5f;
        const _float fAbsX = fabsf(vDirectionValue.x);
        const _float fAbsZ = fabsf(vDirectionValue.z);

        LocalVolume.Extents.y += fBasePadding;

        if (fAbsX >= fAbsZ)
        {
            LocalVolume.Center.x += vDirectionValue.x * fHalfDepth;
            LocalVolume.Extents.x += fHalfDepth + fBasePadding;
            LocalVolume.Extents.z += fSidePadding;
        }
        else
        {
            LocalVolume.Center.z += vDirectionValue.z * fHalfDepth;
            LocalVolume.Extents.z += fHalfDepth + fBasePadding;
            LocalVolume.Extents.x += fSidePadding;
        }
    }

    _float3 vLocalCorners[8] = {};
    _float3 vWorldCorners[8] = {};
    LocalVolume.GetCorners(vLocalCorners);

    _vector vPortalCenter = XMLoadFloat3(&PortalDesc.BoundingPortal.Center);

    for (_uint i = 0; i < 8; ++i)
        XMStoreFloat3(&vWorldCorners[i], XMVector3TransformCoord(XMLoadFloat3(&vLocalCorners[i]), matRotation) + vPortalCenter);

    BoundingBox WorldVolume{};
    BoundingBox::CreateFromPoints(WorldVolume, 8, vWorldCorners, sizeof(_float3));

    return WorldVolume;
}

_int CMapController::Calc_ChunkCoord(_float fPosition) const
{
	// 월드 좌표를 ChunkSize 기준의 정수 Chunk 좌표로 변환한다.
	return static_cast<_int>(floorf(fPosition / m_fChunkSize));
}

BoundingBox CMapController::Make_ChunkCellBounds(_int iChunkX, _int iChunkZ) const
{
	// 청크 좌표에 대응하는 고정 크기 bounds를 구성함
	BoundingBox bounds{};

	// XZ 평면 기준 고정 크기 Chunk Cell Bounds를 만든다.
	bounds.Center = {
		(static_cast<_float>(iChunkX) + 0.5f) * m_fChunkSize,
		0.f,
		(static_cast<_float>(iChunkZ) + 0.5f) * m_fChunkSize
	};

	bounds.Extents = {
		m_fChunkSize * 0.5f,
		10000.f,
		m_fChunkSize * 0.5f
	};

	return bounds;
}

void CMapController::Add_GlobalCandidateIndex(CMapInstanceObject* pOwner, _uint iGlobalInstanceIndex)
{
	if (nullptr == pOwner || UINT_MAX == iGlobalInstanceIndex || nullptr == m_pMapRenderInstancePool)
		return;

	m_pMapRenderInstancePool->Add_Candidate(iGlobalInstanceIndex);

	// 같은 owner는 프레임당 한 번만 global draw 제출 목록에 넣는다.
	if (m_FrameGlobalCSOwnerSet.emplace(pOwner).second)
		m_FrameGlobalCSOwners.push_back(pOwner);
}

void CMapController::Set_SectorRenderFull(_uint iSectorID)
{
	auto Iter = m_Sectors.find(iSectorID);

	if (m_Sectors.end() == Iter)
		return;

	Iter->second.eRenderMode = SECTOR_RENDER_MODE::FULL;
	Iter->second.PortalRenderDescs.clear();
	Iter->second.PortalVolumes.clear();


}

void CMapController::Set_SectorPortalRender(_uint iSectorID, const vector<tSectorPortalRenderDesc>& PortalRenderDescs)
{
	auto Iter = m_Sectors.find(iSectorID);

	if (m_Sectors.end() == Iter)
		return;

	SECTOR_SPATIAL_DATA& SectorData = Iter->second;

	SectorData.eRenderMode = SECTOR_RENDER_MODE::PORTAL_FILTERED;
	SectorData.PortalRenderDescs = PortalRenderDescs;
	SectorData.PortalVolumes.clear();
	SectorData.PortalVolumes.reserve(PortalRenderDescs.size());

	// Late_Update에서 반복 변환하지 않도록 정책 설정 시 PortalVolume을 캐싱한다.
	for (const tSectorPortalRenderDesc& PortalDesc : PortalRenderDescs)
		SectorData.PortalVolumes.push_back(Make_PortalVolume(PortalDesc));
}

void CMapController::Set_SectorRenderHidden(_uint iSectorID)
{
	auto Iter = m_Sectors.find(iSectorID);

	if (m_Sectors.end() == Iter)
		return;

	Iter->second.eRenderMode = SECTOR_RENDER_MODE::HIDDEN;
	Iter->second.PortalRenderDescs.clear();
	Iter->second.PortalVolumes.clear();


}

_bool CMapController::Set_SectorUseHLOD(_uint iSectorID, _bool bUseHLOD)
{
	// Original이 등록되지 않은 섹터는 표현 상태를 변경할 수 없다.
	if (m_Sectors.end() == m_Sectors.find(iSectorID))
		return false;

	const _bool bCurrentlyUsingHLOD = m_HLODRenderSectorIDs.end() != m_HLODRenderSectorIDs.find(iSectorID);

	// 이미 요청한 표현 상태라면 컨테이너 작업을 반복하지 않는다.
	if (bCurrentlyUsingHLOD == bUseHLOD)
		return true;

	if (false == bUseHLOD)
	{
		// 다음 Late_Update부터 Original 공간 구조를 사용한다.
		m_HLODRenderSectorIDs.erase(iSectorID);
		return true;
	}

	auto HLODIter = m_HLODMaps.find(iSectorID);

	// 실제 HLOD가 없는 섹터는 HLOD 상태로 변경하지 않는다.
	if (m_HLODMaps.end() == HLODIter || nullptr == HLODIter->second)
		return false;

	// 다음 Late_Update부터 HLOD만 제출한다.
	m_HLODRenderSectorIDs.emplace(iSectorID);
	return true;
}


CMapController* CMapController::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMapController* pInstance = new CMapController(pDevice, pContext);

	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Created : CMapController");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapController::Free()
{
	Clear_Sectors();
	m_SubmittedObjects.clear();

	Safe_Release(m_pMapRenderInstancePool);

	Safe_Release(m_pMapCullCSConstantBuffer);
	Safe_Release(m_pMapInstanceCullCS);

	__super::Free();
}

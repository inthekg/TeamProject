#include "PhysicManager.h"
#include "GameObject.h"
#include "PXDebugRenderer.h"
#include "Engine_Utility.h"
#include <thread>

CSimulationCallback CPhysicManager::m_gSimulationCallback;
//m_gCCTQueryFilter
CCTQueryFilter CPhysicManager::m_gCCTQueryFilter;
CCTBehaviorCallBack CPhysicManager::m_gCCTBehaviorCallBack;

void CErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	char buffer[1024];
	sprintf_s(buffer, "[PhysX Error] Code: %d | Message: %s | File: %s | Line: %d\n",
		code, message, file, line);

	OutputDebugStringA(buffer);

	if (code == PxErrorCode::eABORT) 
	{
		assert(false);
	}
    
}

/* Filter Shader */
PxFilterFlags CPhysicManager::CustomFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	//둘다 트리거면 무조건 리턴 
	if (PxFilterObjectIsTrigger(attributes0) && PxFilterObjectIsTrigger(attributes1))
	{
		return PxFilterFlag::eSUPPRESS; // 무시 
	}

	; // 충돌 여부 결정. 둘다 0이면 충돌 안함. 하나라도 1이면 충돌함
	_bool bShouldCollide = (filterData0.word0 & filterData1.word1) || (filterData0.word1 & filterData1.word0);
	if(!bShouldCollide)
	{
		return PxFilterFlag::eSUPPRESS; // 무시 
	}

	//하나라도 트리거면 트리거 이벤트
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND; // 충돌 시작 시 콜백 호출
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;  // 충돌 해제 시 콜백 호출
		return PxFilterFlag::eDEFAULT;
	}

	//둘 다 일반 강체라면
	pairFlags = PxPairFlag::eCONTACT_DEFAULT; 
	pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND; 
	pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;  
	//pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS; // 정확한 데이터
	pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT; //CCD 스윕 허용 (모두 허용X Actor에서 따로 설정해야 CCD가능) 

	return PxFilterFlag::eDEFAULT;
}


CPhysicManager::CPhysicManager()
{
    
}


void CPhysicManager::Bind_TriggerCallback(function<void(PxTriggerPair*, PxU32)> func)
{
	m_gSimulationCallback.Bind_TriggerCallback(func);
}

void CPhysicManager::Bind_ContactCallback(function<void(const PxContactPairHeader&, const PxContactPair*, PxU32)> func)
{
	m_gSimulationCallback.Bind_ContactCallback(func);
}

//매 프레임 실행하면서 step Count 초기화 
void CPhysicManager::Accumulate_Time(_float fTimeDelta)
{
	m_iStepCount = 0;
	m_fAccumulatedTime += fTimeDelta;
	if (m_fAccumulatedTime > m_fAccumulatedMaxTime) { m_fAccumulatedTime = m_fAccumulatedMaxTime; }
}

_bool CPhysicManager::Is_AccumulatedTime_Enough() const
{
	return (m_fAccumulatedTime >= m_fStepTime) && (m_iStepCount < m_iMaxStepCount);
}

void CPhysicManager::Start_Simulation()
{
	m_iStepCount++;
	if (m_pCurrentScene == nullptr) return;
	//고정 time
	m_pCurrentScene->simulate(m_fStepTime);

}

void CPhysicManager::Finish_Simulation()
{
	if (m_pCurrentScene == nullptr) return;
	//true : 시뮬레이션이 완료될 때까지 대기함. false : 시뮬레이션이 완료되지 않아도 다음 코드 실행
	m_pCurrentScene->fetchResults(true);
	m_fAccumulatedTime -= m_fStepTime;
}

void CPhysicManager::Simulate_OneStep(_float fTimeDelta)
{
	if (m_pCurrentScene == nullptr) return;
	//HitStop 때는 시뮬레이션 X
	if (fTimeDelta <= 0.f) return;
	m_pCurrentScene->simulate(fTimeDelta);
	m_pCurrentScene->fetchResults(true);
}


bool CPhysicManager::Cook_TriangleMesh(PxTriangleMeshDesc& meshDesc, PxDefaultMemoryOutputStream& OutWriteBuffer)
{
	constexpr PxReal fMinimumTriangleQuality = 0.00005f;

	if (false == meshDesc.isValid())
		return false;

	const _bool bHasTriangleIndices = meshDesc.triangles.data != nullptr;
	const _bool bUse16BitIndices = meshDesc.flags.isSet(PxMeshFlag::e16_BIT_INDICES);
	const PxU32 iSourceTriangleCount = bHasTriangleIndices ? meshDesc.triangles.count : meshDesc.points.count / 3;

	if (0 == iSourceTriangleCount)
		return false;

	// 면별 재질이 존재하면 삼각형 개수만큼 재질 정보가 있어야 한다.
	if (meshDesc.materialIndices.data != nullptr && meshDesc.materialIndices.count < iSourceTriangleCount)
		return false;

	vector<PxU32> FilteredIndices{};
	vector<PxMaterialTableIndex> FilteredMaterialIndices{};

	FilteredIndices.reserve(static_cast<size_t>(iSourceTriangleCount) * 3);

	if (meshDesc.materialIndices.data != nullptr)
		FilteredMaterialIndices.reserve(iSourceTriangleCount);

	PxU32 iRemovedTriangleCount = 0;

	for (PxU32 iTriangleIndex = 0; iTriangleIndex < iSourceTriangleCount; ++iTriangleIndex)
	{
		PxU32 iIndex0 = 0;
		PxU32 iIndex1 = 0;
		PxU32 iIndex2 = 0;

		if (bHasTriangleIndices)
		{
			const PxU8* pTriangleData = reinterpret_cast<const PxU8*>(meshDesc.triangles.data) + static_cast<size_t>(iTriangleIndex) * meshDesc.triangles.stride;

			// 입력 충돌 메시의 인덱스 크기에 맞춰 삼각형을 읽는다.
			if (bUse16BitIndices)
			{
				const PxU16* pIndices = reinterpret_cast<const PxU16*>(pTriangleData);

				iIndex0 = static_cast<PxU32>(pIndices[0]);
				iIndex1 = static_cast<PxU32>(pIndices[1]);
				iIndex2 = static_cast<PxU32>(pIndices[2]);
			}
			else
			{
				const PxU32* pIndices = reinterpret_cast<const PxU32*>(pTriangleData);

				iIndex0 = pIndices[0];
				iIndex1 = pIndices[1];
				iIndex2 = pIndices[2];
			}
		}
		else
		{
			// 비인덱스 메시에서는 연속된 세 정점을 하나의 삼각형으로 취급한다.
			iIndex0 = iTriangleIndex * 3;
			iIndex1 = iIndex0 + 1;
			iIndex2 = iIndex0 + 2;
		}

		// 범위를 벗어난 인덱스는 메시 데이터 손상이므로 쿠킹을 중단한다.
		if (iIndex0 >= meshDesc.points.count || iIndex1 >= meshDesc.points.count || iIndex2 >= meshDesc.points.count)
			return false;

		const PxVec3& vPoint0 = meshDesc.points.at<PxVec3>(iIndex0);
		const PxVec3& vPoint1 = meshDesc.points.at<PxVec3>(iIndex1);
		const PxVec3& vPoint2 = meshDesc.points.at<PxVec3>(iIndex2);

		// NaN 또는 무한대 좌표를 PhysX 쿠킹에 전달하지 않는다.
		if (!vPoint0.isFinite() || !vPoint1.isFinite() || !vPoint2.isFinite())
			return false;

		const PxVec3 vEdge01 = vPoint1 - vPoint0;
		const PxVec3 vEdge02 = vPoint2 - vPoint0;
		const PxVec3 vEdge12 = vPoint2 - vPoint1;

		const PxReal fEdge01Squared = vEdge01.magnitudeSquared();
		const PxReal fEdge02Squared = vEdge02.magnitudeSquared();
		const PxReal fEdge12Squared = vEdge12.magnitudeSquared();

		PxReal fMaximumEdgeSquared = fEdge01Squared;

		if (fEdge02Squared > fMaximumEdgeSquared)
			fMaximumEdgeSquared = fEdge02Squared;

		if (fEdge12Squared > fMaximumEdgeSquared)
			fMaximumEdgeSquared = fEdge12Squared;

		// 세 정점이 같은 위치에 있는 완전 퇴화 삼각형을 제거한다.
		if (fMaximumEdgeSquared <= 0.f)
		{
			++iRemovedTriangleCount;
			continue;
		}

		// 스케일 영향을 받지 않도록 넓이와 가장 긴 변의 비율로 품질을 계산한다.
		const PxReal fDoubleArea = vEdge01.cross(vEdge02).magnitude();
		const PxReal fTriangleQuality = fDoubleArea / fMaximumEdgeSquared;

		// 길이에 비해 지나치게 얇은 슬리버 삼각형을 충돌 메시에서 제거한다.
		if (!PxIsFinite(fTriangleQuality) || fTriangleQuality < fMinimumTriangleQuality)
		{
			++iRemovedTriangleCount;
			continue;
		}

		// 정상 삼각형의 인덱스와 winding order는 그대로 유지한다.
		FilteredIndices.push_back(iIndex0);
		FilteredIndices.push_back(iIndex1);
		FilteredIndices.push_back(iIndex2);

		// 면별 재질이 있으면 제거되지 않은 삼각형의 재질도 함께 복사한다.
		if (meshDesc.materialIndices.data != nullptr)
			FilteredMaterialIndices.push_back(meshDesc.materialIndices.at(iTriangleIndex));
	}

	// 유효한 삼각형이 하나도 남지 않았다면 충돌 메시를 생성하지 않는다.
	if (FilteredIndices.empty())
		return false;

	static PxCookingParams gCookingParams(m_Default_TolerancesScale);

	// 기존 중복 정점 병합 설정을 유지한다.
	gCookingParams.meshPreprocessParams |= PxMeshPreprocessingFlag::eWELD_VERTICES;
	gCookingParams.meshWeldTolerance = 0.001f;

	// 기존 BVH34 충돌 가속 구조를 유지한다.
	gCookingParams.midphaseDesc.setToDefault(PxMeshMidPhase::eBVH34);

	// 제거할 삼각형이 없으면 원본 descriptor를 그대로 사용한다.
	if (0 == iRemovedTriangleCount)
		return PxCookTriangleMesh(gCookingParams, meshDesc, OutWriteBuffer);

	PxTriangleMeshDesc FilteredMeshDesc = meshDesc;

	FilteredMeshDesc.triangles.count = static_cast<PxU32>(FilteredIndices.size() / 3);
	FilteredMeshDesc.triangles.stride = sizeof(PxU32) * 3;
	FilteredMeshDesc.triangles.data = FilteredIndices.data();

	// 필터링 결과 인덱스는 항상 PxU32 형식이다.
	FilteredMeshDesc.flags.clear(PxMeshFlag::e16_BIT_INDICES);

	if (meshDesc.materialIndices.data != nullptr)
	{
		FilteredMeshDesc.materialIndices.count = static_cast<PxU32>(FilteredMaterialIndices.size());
		FilteredMeshDesc.materialIndices.stride = sizeof(PxMaterialTableIndex);
		FilteredMeshDesc.materialIndices.data = FilteredMaterialIndices.data();
	}

	return PxCookTriangleMesh(gCookingParams, FilteredMeshDesc, OutWriteBuffer);
}

void CPhysicManager::Reset_Filtering(PxActor* pActor)
{
    if (m_pCurrentScene == nullptr || pActor == nullptr) return;
    m_pCurrentScene->resetFiltering(*pActor);
}

void CPhysicManager::Invalidate_AllControllerCache()
{
	if (!m_pCurrentControllerManager) return;
	for(PxU32 i=0; i< m_pCurrentControllerManager->getNbControllers(); ++i)
	{
		PxController* pController = m_pCurrentControllerManager->getController(i);
		if (pController)
		{
			pController->invalidateCache();
		}
	}
}



PxScene* CPhysicManager::Create_PxScene(const PxSceneDesc& desc)
{
	if (m_pPhysics == nullptr) return nullptr;
	if (m_pCurrentScene != nullptr)
	{
		m_pCurrentScene->release();
		if(m_pCurrentControllerManager) m_pCurrentControllerManager->release();
	}
    if (!desc.isValid()) return nullptr;

	m_pCurrentScene = m_pPhysics->createScene(desc);
	if (m_pCurrentScene == nullptr) return nullptr;

#ifdef _DEBUG
	//Debug 시각화 설정. 
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.f);
	//Shape 외곽선 그리기 On
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
	//Actor의 local axis 그리기 On
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
	//충돌 지점 그리기 On
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);

#endif // _DEBUG

	m_pCurrentControllerManager = PxCreateControllerManager(*m_pCurrentScene);
	if (m_pCurrentControllerManager == nullptr)
	{
		MSG_BOX("Current Controller Manager Create Fail");
	}
	//끼었을때 (CCT가 다른 콜라이더에 overlap) 회복하는 (빠져나오는) 기능 On
	m_pCurrentControllerManager->setOverlapRecoveryModule(true);

	m_pCurrentScene->setSimulationEventCallback(&m_gSimulationCallback);

	return m_pCurrentScene;
}

//isExclusive : true면 하나의 강체에만 종속됨.
PxShape* CPhysicManager::Create_PxShape(const PxGeometry& geometry, PxMaterial* material, bool isExclusive)
{
    return m_pPhysics->createShape(geometry, *material, isExclusive);
}

PxMaterial* CPhysicManager::Create_PxMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	if (m_pPhysics == nullptr) return nullptr;	

	PxMaterial* pMaterial = m_pPhysics->createMaterial(staticFriction, dynamicFriction, restitution);
	return pMaterial;
}

PxRigidDynamic* CPhysicManager::Create_RigidDynamic(_fvector vInitPos)
{
    PxTransform transform = PxTransform(PxVec3(XMVectorGetX(vInitPos), XMVectorGetY(vInitPos), XMVectorGetZ(vInitPos)));
    return m_pPhysics->createRigidDynamic(transform);
}

PxTriangleMesh* CPhysicManager::Create_TriangleMesh(PxInputStream& stream)
{
    return m_pPhysics->createTriangleMesh(stream);
}

PxRigidStatic* CPhysicManager::Create_RigidStatic(_fvector vInitPos, _fvector vInitQuat)
{
    PxTransform transform = PxTransform(PxVec3(XMVectorGetX(vInitPos), XMVectorGetY(vInitPos), XMVectorGetZ(vInitPos)),
        PxQuat(XMVectorGetX(vInitQuat), XMVectorGetY(vInitQuat), XMVectorGetZ(vInitQuat), XMVectorGetW(vInitQuat)));
    return m_pPhysics->createRigidStatic(transform);
}

bool CPhysicManager::Raycast(const XMFLOAT3& vRayOrigin, const XMFLOAT3& vRayDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	if (m_pCurrentScene == nullptr) return false;

	PxVec3 pxOrigin(vRayOrigin.x, vRayOrigin.y, vRayOrigin.z);
	PxVec3 pxDir(vRayDir.x, vRayDir.y, vRayDir.z);
	PxReal pxDist = fDistance;

	pxDir.normalize();

	PxQueryFilterData queryFilterData;
	queryFilterData.data.word1 = ETOUI(eTargetFilter);
	queryFilterData.flags = QueryFlags | PxQueryFlag::ePREFILTER | PxQueryFlag::eDISABLE_HARDCODED_FILTER;

	PxRaycastBuffer pxHitResult = {};

	if (m_pCurrentScene->raycast(pxOrigin, pxDir, pxDist, pxHitResult,PxHitFlags(PxHitFlag::eDEFAULT),queryFilterData, &m_gCCTQueryFilter))
	{
		OutResult.eCollisionType = COLLISION_TYPE::RAYCAST;
		OutResult.tRaycastData.vHitPosition = XMFLOAT3(pxHitResult.block.position.x, pxHitResult.block.position.y, pxHitResult.block.position.z);
		OutResult.pOtherActor = static_cast<CGameObject*>(pxHitResult.block.actor->userData);
		OutResult.tRaycastData.fDistance = pxHitResult.block.distance;

		return true;
	}
	return false;
}

_bool CPhysicManager::Sweep_Sphere(_float fRadius, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	if (m_pCurrentScene == nullptr) return false;

	PxVec3 origin(XMVectorGetX(vOrigin), XMVectorGetY(vOrigin), XMVectorGetZ(vOrigin));
	PxVec3 dir(XMVectorGetX(vDir), XMVectorGetY(vDir), XMVectorGetZ(vDir));
	if (dir.magnitudeSquared() < g_fEpsilon) return false;
	dir.normalize();

	PxSphereGeometry Sphere(fRadius);
	PxTransform Pos(origin);

	PxQueryFilterData queryFilterData;
	queryFilterData.data.word1 = ETOUI(eTargetFilter);
	queryFilterData.flags = QueryFlags | PxQueryFlag::ePREFILTER | PxQueryFlag::eDISABLE_HARDCODED_FILTER;

	PxSweepBuffer pxHitResult = {};
	const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMTD;

	if(m_pCurrentScene->sweep(Sphere, Pos, dir, fDistance, pxHitResult, hitFlags, queryFilterData, &m_gCCTQueryFilter))
	{
		OutResult.eCollisionType = COLLISION_TYPE::SWEEP;
		OutResult.pOtherActor = static_cast<CGameObject*>(pxHitResult.block.actor->userData);
		OutResult.tSweepData.vHitPosition = XMFLOAT3(pxHitResult.block.position.x, pxHitResult.block.position.y, pxHitResult.block.position.z);
		OutResult.tSweepData.vHitNormal = XMFLOAT3(pxHitResult.block.normal.x, pxHitResult.block.normal.y, pxHitResult.block.normal.z);
		OutResult.tSweepData.fDistance = pxHitResult.block.distance;
		OutResult.tSweepData.pHitShape = pxHitResult.block.shape;
		OutResult.tSweepData.vHitDirection = XMFLOAT3(dir.x, dir.y, dir.z);

		return true;
	}
	return false;
}


_bool CPhysicManager::Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	if (m_pCurrentScene == nullptr) return false;

	PxVec3 origin(XMVectorGetX(vOrigin), XMVectorGetY(vOrigin), XMVectorGetZ(vOrigin));
	PxVec3 dir(XMVectorGetX(vDir), XMVectorGetY(vDir), XMVectorGetZ(vDir));
	if (dir.magnitudeSquared() < g_fEpsilon) return false;
	dir.normalize();

	PxCapsuleGeometry Capsule(fRadius, fHalfHeight);
	PxTransform Pos(origin);

	PxQueryFilterData queryFilterData;
	queryFilterData.data.word1 = ETOUI(eTargetFilter);
	queryFilterData.flags = QueryFlags | PxQueryFlag::ePREFILTER | PxQueryFlag::eDISABLE_HARDCODED_FILTER;

	PxSweepBuffer pxHitResult = {};
	const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMTD;

	if (m_pCurrentScene->sweep(Capsule, Pos, dir, fDistance, pxHitResult, hitFlags, queryFilterData, &m_gCCTQueryFilter))
	{
		OutResult.eCollisionType = COLLISION_TYPE::SWEEP;
		OutResult.pOtherActor = static_cast<CGameObject*>(pxHitResult.block.actor->userData);
		OutResult.tSweepData.vHitPosition = XMFLOAT3(pxHitResult.block.position.x, pxHitResult.block.position.y, pxHitResult.block.position.z);
		OutResult.tSweepData.vHitNormal = XMFLOAT3(pxHitResult.block.normal.x, pxHitResult.block.normal.y, pxHitResult.block.normal.z);
		OutResult.tSweepData.fDistance = pxHitResult.block.distance;
		OutResult.tSweepData.pHitShape = pxHitResult.block.shape;
		OutResult.tSweepData.vHitDirection = XMFLOAT3(dir.x, dir.y, dir.z);

		return true;
	}
	return false;
}


_bool CPhysicManager::Sweep_Capsule(_float fRadius, _float fHalfHeight, _fvector vOrigin, _fvector vQuat, _fvector vDir, _float fDistance, FILTER_GROUP eTargetFilter, CollisionResult& OutResult, PxQueryFlags QueryFlags)
{
	if (m_pCurrentScene == nullptr) return false;

	PxVec3 origin(XMVectorGetX(vOrigin), XMVectorGetY(vOrigin), XMVectorGetZ(vOrigin));
	PxVec3 dir(XMVectorGetX(vDir), XMVectorGetY(vDir), XMVectorGetZ(vDir));
	if (dir.magnitudeSquared() < g_fEpsilon) return false;
	dir.normalize();

	PxQuat quat(XMVectorGetX(vQuat), XMVectorGetY(vQuat), XMVectorGetZ(vQuat), XMVectorGetW(vQuat));

	PxCapsuleGeometry Capsule(fRadius, fHalfHeight);
	PxTransform Pos(origin, quat);

	PxQueryFilterData queryFilterData;
	queryFilterData.data.word1 = ETOUI(eTargetFilter);
	queryFilterData.flags = QueryFlags | PxQueryFlag::ePREFILTER | PxQueryFlag::eDISABLE_HARDCODED_FILTER;

	PxSweepBuffer pxHitResult = {};
	const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMTD;

	if (m_pCurrentScene->sweep(Capsule, Pos, dir, fDistance, pxHitResult, hitFlags, queryFilterData, &m_gCCTQueryFilter))
	{
		OutResult.eCollisionType = COLLISION_TYPE::SWEEP;
		OutResult.pOtherActor = static_cast<CGameObject*>(pxHitResult.block.actor->userData);
		OutResult.tSweepData.vHitPosition = XMFLOAT3(pxHitResult.block.position.x, pxHitResult.block.position.y, pxHitResult.block.position.z);
		OutResult.tSweepData.vHitNormal = XMFLOAT3(pxHitResult.block.normal.x, pxHitResult.block.normal.y, pxHitResult.block.normal.z);
		OutResult.tSweepData.fDistance = pxHitResult.block.distance;
		OutResult.tSweepData.pHitShape = pxHitResult.block.shape;
		OutResult.tSweepData.vHitDirection = XMFLOAT3(dir.x, dir.y, dir.z);

		return true;
	}
	return false;
}

_bool CPhysicManager::Overlap_Sphere(_float fRadius, _fvector vOrigin, FILTER_GROUP eTargetFilter, vector<CGameObject*>& OutResults, PxQueryFlags QueryFlags)
{
	if (m_pCurrentScene == nullptr) return false;

	PxVec3 origin(XMVectorGetX(vOrigin), XMVectorGetY(vOrigin), XMVectorGetZ(vOrigin));
	PxSphereGeometry Sphere(fRadius);
	PxTransform Pose(origin);

	PxQueryFilterData queryFilterData;
	queryFilterData.data.word1 = ETOUI(eTargetFilter);

	// eNO_BLOCK : Block이 있어도 탐색을 중지하지말고 계속 이어나가라는 플래그 
	// 원래 BLOCK이 있으면 탐색을 중지하고 결과 반환함. eTOUCH는 탐지만 
	queryFilterData.flags = QueryFlags | PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK | PxQueryFlag::eDISABLE_HARDCODED_FILTER;

	PxOverlapBuffer pxHitResult = {};
	const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMTD;

	const PxU32 BUFFER_SIZE = 64;
	PxOverlapHit hitBuffer[BUFFER_SIZE];
	PxOverlapBuffer overlapBuf(hitBuffer, BUFFER_SIZE);

	m_pCurrentScene->overlap(Sphere, Pose, overlapBuf, queryFilterData, &m_gCCTQueryFilter);

	OutResults.clear();
	for (PxU32 i = 0; i < overlapBuf.nbTouches; ++i)
	{
		PxActor* pActor = overlapBuf.touches[i].actor;
		if (pActor)
		{
			CGameObject* pGameObject = static_cast<CGameObject*>(pActor->userData);
			if (pGameObject == nullptr) continue;

			auto iter = find(OutResults.begin(), OutResults.end(), pGameObject);
			if (iter != OutResults.end()) continue;

			OutResults.push_back(pGameObject);	
		}
	}

	return !OutResults.empty();	
}

void CPhysicManager::Add_ActorToScene(PxActor* pActor)
{
    m_pCurrentScene->addActor(*pActor);
}

void CPhysicManager::Remove_ActorFromScene(PxActor* pActor)
{
    if (m_pCurrentScene && pActor)
        m_pCurrentScene->removeActor(*pActor);
}

HRESULT CPhysicManager::Initialize()
{	
    //게임의 "단위"를 설정함. 1유닛 = 1미터, 1속도 1m/s 가 디폴트. 
    //speed : 평균 속도. 이 만큼 움직였다면 이동한것이다 라고 판단 

	m_Default_TolerancesScale.length = 1.f;
	m_Default_TolerancesScale.speed = 10.f; //Default 값 

	static CErrorCallback ErrorCallback;
	m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_DefaultAllocator, ErrorCallback);

	if(m_pFoundation == nullptr)
		return E_FAIL;

	//Foundation 생성 후 호출하기 
    //물리연산에 사용할 CPU스레드 개수 
	PxU32 piOptimalThreadCount = Get_OptimalPhysicsThreadCount();
	m_pDefault_CpuDispatcher = PxDefaultCpuDispatcherCreate(piOptimalThreadCount);	

	if (m_pDefault_CpuDispatcher == nullptr)
		return E_FAIL;

    //tackOutstanding: 피직스에서 할당한 모든 메모리를 추적. 디버그에서만 true로 설정
	_bool bTrackOutstandingAllocations = false;

#ifdef _DEBUG
	bTrackOutstandingAllocations = true;
#endif

	m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, m_Default_TolerancesScale, bTrackOutstandingAllocations, nullptr);

	if(m_pPhysics == nullptr)
		return E_FAIL;
	
	//조인트 기능 사용 
	if(!PxInitExtensions(*m_pPhysics, nullptr))
		return E_FAIL;

	m_pDefaultMaterial = Create_PxMaterial(0.5f, 0.5f, 0.5f);
	if (m_pDefaultMaterial == nullptr) return E_FAIL;

	return S_OK;
}


PxU32 CPhysicManager::Get_OptimalPhysicsThreadCount()
{
    //하드웨어가 지원하는 논리 코어 수 
    PxU32 iHardwareThreads = thread::hardware_concurrency();

    //코어수가 너무 적으면 최소 1 반환
    if (iHardwareThreads <= 2) return 1;
    PxU32 optimalThreads = iHardwareThreads - 2;
    return iClamp(optimalThreads, 1u, 4u);
}


/* Debug Renderer */

#ifdef _DEBUG
HRESULT CPhysicManager::Create_DebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (m_pDebugRenderer != nullptr) return S_OK;

    m_pDebugRenderer = CPXDebugRenderer::Create(pDevice, pContext);
    if(m_pDebugRenderer == nullptr)
    {
        MSG_BOX("Failed to Create Debug Renderer");
        return E_FAIL;
    }
    return S_OK;
}

void CPhysicManager::On_VisualDebugger()
{
    if (m_pCurrentScene == nullptr) return;
    m_bIsVisualDebuggerOn = true; 
    //시각화 스케일 설정 (0으로 설정하면 시각화가 꺼짐)
    m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f); 
    //충돌체 모양 그리기 키기 
    m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, m_fColliderLineWidth);
	//Actor의 local axis 그리기 On
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
	//충돌 지점 그리기 On
	m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);
}

void CPhysicManager::Off_VisualDebugger()
{
    if (m_pCurrentScene == nullptr) return;
    m_bIsVisualDebuggerOn = false;
    m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
    m_pCurrentScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 0.f);
}

void CPhysicManager::Toggle_VisualDebugger()
{
    if(m_bIsVisualDebuggerOn) Off_VisualDebugger();
    else On_VisualDebugger();   
}

void CPhysicManager::VisualDebugger_Update()
{
    if(m_pDebugRenderer == nullptr || m_bIsVisualDebuggerOn == false) return;
	m_pDebugRenderer->Update();
}

const PxRenderBuffer& CPhysicManager::Get_RenderBuffer() const
{
    return m_pCurrentScene->getRenderBuffer();
}

void CPhysicManager::Apply_DebugVisible(_bool bVisible)
{
	if (m_pCurrentScene == nullptr) return;

	const PxActorTypeFlags TypeFlags = PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC;
	const PxU32 iCount = m_pCurrentScene->getNbActors(TypeFlags);
	if (iCount == 0) return;

	vector<PxActor*> Actors(iCount, nullptr);
	m_pCurrentScene->getActors(TypeFlags, Actors.data(), iCount);

	for (PxActor* pActor : Actors)
	{
		if (pActor == nullptr) continue;
		//지형은 제외. 지형은 별도로 랜더 토글함 
		if (pActor->userData == nullptr) continue;

		CGameObject* pObject = static_cast<CGameObject*>(pActor->userData);
		if (pObject->Is_InActive()) continue;

		pActor->setActorFlag(PxActorFlag::eVISUALIZATION, bVisible);
	}
}

#endif

CPhysicManager*  CPhysicManager::Create()
{
	CPhysicManager* pInstance = new CPhysicManager();
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CPhysicManager");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CPhysicManager::Free()
{
    Safe_Release(m_pDebugRenderer);

	//Scene이 Physics보다 먼저 해제되어야 함. Scene이 Physics에 접근할 수 있기 때문
	// 순서 주의 
	if (m_pCurrentControllerManager) m_pCurrentControllerManager->release();
	if (m_pCurrentScene) m_pCurrentScene->release();
	
	PxCloseExtensions();
	if (m_pPhysics) m_pPhysics->release();
	//Delete로 지워야함 
	if (m_pDefault_CpuDispatcher) delete m_pDefault_CpuDispatcher;
	if (m_pFoundation) m_pFoundation->release();
}


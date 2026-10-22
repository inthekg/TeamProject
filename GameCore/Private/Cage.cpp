#include "Cage.h"

#include "Animator.h"
#include "Model.h"
#include "DynamicRigidBody.h"
#include "GameInstance.h"

CCage::CCage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CCage::CCage(const CCage& Prototype)
	: CMapGameplayObject{ Prototype }
{
}

HRESULT CCage::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CCage::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	MAP_GAMEPLAY_OBJECT_DESC Desc = *static_cast<MAP_GAMEPLAY_OBJECT_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);

	if (true == Desc.bUsePhysics)
	{
		if (false == std::isfinite(Desc.vPhysicsHalfExtents.x) || false == std::isfinite(Desc.vPhysicsHalfExtents.y) || false == std::isfinite(Desc.vPhysicsHalfExtents.z))
			return E_FAIL;

		if (false == std::isfinite(Desc.vPhysicsLocalOffset.x) || false == std::isfinite(Desc.vPhysicsLocalOffset.y) || false == std::isfinite(Desc.vPhysicsLocalOffset.z))
			return E_FAIL;

		if (Desc.vPhysicsHalfExtents.x <= 0.f || Desc.vPhysicsHalfExtents.y <= 0.f || Desc.vPhysicsHalfExtents.z <= 0.f)
			return E_FAIL;
	}

	// Cage 바닥 위치에 전용 Dynamic Rigidbody를 생성하므로 공통 Static Rigidbody는 생성하지 않는다.
	MAP_GAMEPLAY_OBJECT_DESC BaseDesc = Desc;
	BaseDesc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	if (nullptr == m_pAnimator || nullptr == m_pModelCom)
		return E_FAIL;

	// Cage SpawnData의 TargetSpawnID는 열릴 때 깨울 Monster SpawnID로 사용한다.
	m_iTargetMonsterSpawnID = Desc.iTargetSpawnID;

	// Controller Entry 상태와 닫힌 바닥 본 Pose를 첫 물리 갱신 전에 확정한다.
	Set_AnimBool(TEXT("Open"), false);
	Drive_Animator(0.f);
	m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	if (false == Is_AnimInState(TEXT("Close_Idle")))
		return E_FAIL;

	m_eState = CAGE_STATE::CLOSED;

	if (true == Desc.bUsePhysics && FAILED(Ready_FloorPhysics(Desc)))
		return E_FAIL;

	return S_OK;
}

void CCage::Ready_DefaultDesc(MAP_GAMEPLAY_OBJECT_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	// Cage는 Controller 기반으로 Close_Idle에서 시작하고 Open 후 Open_Idle을 유지한다.
	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Cage") : pDesc->strModelPrototypeTag;
	pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Cage") : pDesc->strAnimFolderTag;
	pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Cage") : pDesc->strAnimTag;
	pDesc->eAnimTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	pDesc->bStaticAnimation = true;
	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
}

HRESULT CCage::Ready_FloorPhysics(const MAP_GAMEPLAY_OBJECT_DESC& Desc)
{
	if (nullptr != m_pCageDynamicRigidBody)
		return S_OK;

	if (nullptr == m_pGameInstance || nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return E_FAIL;

	// 닫힌 Cage의 바닥 전용 본을 기준으로 Dynamic Actor의 월드 Pose를 만든다.
	const _float4x4* pFloorBoneCombinedMatrix = m_pModelCom->Get_Bone_CombinedMatPtr("CageHatch_Ctrl");

	if (nullptr == pFloorBoneCombinedMatrix)
		return E_FAIL;

	const _matrix FloorWorldMatrix = XMLoadFloat4x4(pFloorBoneCombinedMatrix) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	_vector vFloorScale{};
	_vector vFloorQuaternion{};
	_vector vFloorPosition{};

	if (false == XMMatrixDecompose(&vFloorScale, &vFloorQuaternion, &vFloorPosition, FloorWorldMatrix))
		return E_FAIL;

	if (XMVectorGetX(XMVector3LengthSq(vFloorScale)) <= 1e-8f || XMVectorGetX(XMVector4LengthSq(vFloorQuaternion)) <= 1e-8f)
		return E_FAIL;

	// Cage 물리는 외부 Desc 필터에 의존하지 않고 Monster CCT와 직접 충돌하도록 고정한다.
	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;
	// 닫힌 동안 외력과 중력으로 움직이지 않는 Kinematic Dynamic Actor로 유지한다.
	PhysicDesc.bIsKinematic = true;
	PhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::GROUND);
	PhysicDesc.iFilterWord1 = ETOUI(FILTER_GROUP::MONSTER);
	XMStoreFloat3(&PhysicDesc.vInitPos, vFloorPosition);
	XMStoreFloat4(&PhysicDesc.vInitQuat, XMQuaternionNormalize(vFloorQuaternion));

	// 바닥은 Zombie CCT 발 위치보다 아래에 유지해 초기 겹침 없이 낙하를 막는다.
	SHAPE_DESC FloorShape{};
	FloorShape.eType = GEOMETRY_TYPE::BOX;
	FloorShape.vHalfExtents = Desc.vPhysicsHalfExtents;
	FloorShape.vLocalOffset = Desc.vPhysicsLocalOffset;
	FloorShape.bTrigger = false;
	PhysicDesc.Shapes.push_back(FloorShape);

	// 바닥을 높이지 않고 외곽 벽 4개로만 Monster의 수평 탈출을 막는다.
	const _float fWallHalfThicknessX = 0.1f * fabsf(Desc.vScale.x);
	const _float fWallHalfThicknessZ = 0.1f * fabsf(Desc.vScale.z);
	const _float fWallOutsetX = 0.2f * fabsf(Desc.vScale.x);
	const _float fWallOutsetZ = 0.2f * fabsf(Desc.vScale.z);
	const _float fWallHalfHeight = 1.5f * fabsf(Desc.vScale.y);
	const _float fWallCenterLocalY = Desc.vPhysicsLocalOffset.y + Desc.vPhysicsHalfExtents.y + fWallHalfHeight;

	SHAPE_DESC WallShapeX{};
	WallShapeX.eType = GEOMETRY_TYPE::BOX;
	WallShapeX.vHalfExtents = _float3(fWallHalfThicknessX, fWallHalfHeight, Desc.vPhysicsHalfExtents.z + fWallOutsetZ + fWallHalfThicknessZ);
	WallShapeX.vLocalOffset = _float3(Desc.vPhysicsLocalOffset.x + Desc.vPhysicsHalfExtents.x + fWallOutsetX + fWallHalfThicknessX, fWallCenterLocalY, Desc.vPhysicsLocalOffset.z);
	WallShapeX.bTrigger = false;
	PhysicDesc.Shapes.push_back(WallShapeX);

	WallShapeX.vLocalOffset.x = Desc.vPhysicsLocalOffset.x - Desc.vPhysicsHalfExtents.x - fWallOutsetX - fWallHalfThicknessX;
	PhysicDesc.Shapes.push_back(WallShapeX);

	SHAPE_DESC WallShapeZ{};
	WallShapeZ.eType = GEOMETRY_TYPE::BOX;
	WallShapeZ.vHalfExtents = _float3(Desc.vPhysicsHalfExtents.x + fWallOutsetX + fWallHalfThicknessX, fWallHalfHeight, fWallHalfThicknessZ);
	WallShapeZ.vLocalOffset = _float3(Desc.vPhysicsLocalOffset.x, fWallCenterLocalY, Desc.vPhysicsLocalOffset.z + Desc.vPhysicsHalfExtents.z + fWallOutsetZ + fWallHalfThicknessZ);
	WallShapeZ.bTrigger = false;
	PhysicDesc.Shapes.push_back(WallShapeZ);

	WallShapeZ.vLocalOffset.z = Desc.vPhysicsLocalOffset.z - Desc.vPhysicsHalfExtents.z - fWallOutsetZ - fWallHalfThicknessZ;
	PhysicDesc.Shapes.push_back(WallShapeZ);

	// Cage만 사용하는 Dynamic Actor를 직접 생성해 공통 Static 물리와 분리한다.
	m_pCageDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pCageDynamicRigidBody)
		return E_FAIL;

	if (FAILED(m_pCageDynamicRigidBody->Initialize(&PhysicDesc)))
	{
		Safe_Release(m_pCageDynamicRigidBody);
		return E_FAIL;
	}

	PxRigidActor* pCageActor = m_pCageDynamicRigidBody->Get_RigidActor();

	if (nullptr == pCageActor)
	{
		Safe_Release(m_pCageDynamicRigidBody);
		return E_FAIL;
	}

	if (false == Desc.bPhysicsStartRegistered)
	{
		if (nullptr != pCageActor->getScene())
			m_pGameInstance->Remove_ActorFromScene(pCageActor);

		return S_OK;
	}

	if (nullptr == pCageActor->getScene())
		m_pGameInstance->Add_ActorToScene(pCageActor);

	if (nullptr == pCageActor->getScene())
		return E_FAIL;

	// Monster CCT가 Cage보다 먼저 생성되므로 새 Dynamic Shape를 다음 이동 쿼리에 즉시 반영한다.
	m_pGameInstance->Invalidate_AllControllerCache();

	return S_OK;
}

HRESULT CCage::Open()
{
	if (CAGE_STATE::CLOSED != m_eState)
		return S_OK;

	if (nullptr == m_pAnimator)
		return E_FAIL;

	// 바닥이 열리는 즉시 Dynamic 충돌 전체를 제거해서 Cage 내부의 Monster가 아래로 낙하하게 한다.
	if (nullptr != m_pCageDynamicRigidBody)
	{
		PxRigidActor* pCageActor = m_pCageDynamicRigidBody->Get_RigidActor();

		if (nullptr != pCageActor && nullptr != pCageActor->getScene())
			m_pGameInstance->Remove_ActorFromScene(pCageActor);
	}

	// 제거된 Cage Shape가 Controller 캐시에 남지 않도록 같은 프레임에 모든 CCT 캐시를 비운다.
	if (nullptr != m_pGameInstance)
		m_pGameInstance->Invalidate_AllControllerCache();

	m_eState = CAGE_STATE::OPENING;

	// Open은 Request 진입 후 끝까지 재생되고 Open_Idle Auto 상태로 이어진다.
	Set_AnimBool(TEXT("Open"), true);
	Request_AnimState(TEXT("Open"));

	if (nullptr != m_pGameInstance && UINT_MAX != m_iTargetMonsterSpawnID)
	{
		// 같은 Trigger에 연결된 Cage가 지정한 Monster 하나만 깨운다.
		CAGE_OPEN_EVENT_DESC EventDesc{};
		EventDesc.iTargetMonsterSpawnID = m_iTargetMonsterSpawnID;
		m_pGameInstance->Broadcast(EVENT_TYPE_MAP_OBJECT::CAGE_OPEN, &EventDesc);
	}

	Deactivate_LinkedTriggers();

	return S_OK;
}

void CCage::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	Open();
}

void CCage::On_LinkedActivate()
{
	Open();
}

void CCage::Update_MapGameplayObject(_float fTimeDelta)
{
	// Open 애니메이션이 끝나면 재활성화되지 않는 최종 상태로 전환한다.
	if (CAGE_STATE::OPENING == m_eState && true == Is_AnimationFinished())
		Enter_Opened();
}

void CCage::Enter_Opened()
{
	m_eState = CAGE_STATE::OPENED;
}

CCage* CCage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCage* pInstance = new CCage(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CCage");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCage::Clone(void* pArg)
{
	CCage* pInstance = new CCage(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CCage");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCage::Free()
{
	Safe_Release(m_pCageDynamicRigidBody);

	__super::Free();
}

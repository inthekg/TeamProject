#include "MapTrap.h"
#include "Animator.h"
#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "Model.h"
#include "Player.h"
#include "Transform.h"

CMapTrap::CMapTrap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CMapTrap::CMapTrap(const CMapTrap& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eState{ TRAP_STATE::END }
{
}

HRESULT CMapTrap::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CMapTrap::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	MAP_TRAP_DESC Desc = *static_cast<MAP_TRAP_DESC*>(pArg);
	Ready_DefaultDesc(&Desc);

	if (false == std::isfinite(Desc.fDamage) || Desc.fDamage < 0.f)
		return E_FAIL;

	if (false == std::isfinite(Desc.fPushDistance) || Desc.fPushDistance < 0.f)
		return E_FAIL;

	if (false == std::isfinite(Desc.vPhysicsHalfExtents.x) || false == std::isfinite(Desc.vPhysicsHalfExtents.y) || false == std::isfinite(Desc.vPhysicsHalfExtents.z))
		return E_FAIL;

	if (false == std::isfinite(Desc.vPhysicsLocalOffset.x) || false == std::isfinite(Desc.vPhysicsLocalOffset.y) || false == std::isfinite(Desc.vPhysicsLocalOffset.z))
		return E_FAIL;

	if (Desc.vPhysicsHalfExtents.x <= 0.f || Desc.vPhysicsHalfExtents.y <= 0.f || Desc.vPhysicsHalfExtents.z <= 0.f)
		return E_FAIL;

	if (false == std::isfinite(Desc.fBlockingRadius) || false == std::isfinite(Desc.fBlockingHalfHeight))
		return E_FAIL;

	if (false == std::isfinite(Desc.vBlockingLocalOffset.x) || false == std::isfinite(Desc.vBlockingLocalOffset.y) || false == std::isfinite(Desc.vBlockingLocalOffset.z))
		return E_FAIL;

	if (Desc.fBlockingRadius <= 0.f || Desc.fBlockingHalfHeight <= 0.f)
		return E_FAIL;

	// 끝 피해 Trigger와 원통 본체 Blocking Capsule은 각각 Bone을 따라가는 Dynamic Kinematic으로 생성한다.
	MAP_GAMEPLAY_OBJECT_DESC BaseDesc = Desc;
	BaseDesc.bUsePhysics = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	if (nullptr == m_pAnimator || nullptr == m_pModelCom)
		return E_FAIL;

	// 첫 Update 전에 Controller Entry State와 Bone Pose를 확정한다.
	m_pAnimator->Update(0.f);
	m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	if (false == Is_AnimInState(TEXT("WallTrap_Idle")))
		return E_FAIL;

	m_fDamage = Desc.fDamage;
	m_fPushDistance = Desc.fPushDistance;

	// Editor Preview는 모델과 애니메이션만 만들고 Runtime에서만 피격 Trigger를 생성한다.
	if (true == Desc.bPhysicsStartRegistered && FAILED(Ready_DynamicPhysics(Desc)))
		return E_FAIL;

	return Enter_Idle();
}

void CMapTrap::Ready_DefaultDesc(MAP_TRAP_DESC* pDesc)
{
	if (nullptr == pDesc)
		return;

	// Monastery_WallDeco01 모델과 Controller를 설정한다.
	pDesc->eVisualType = VISUAL_TYPE::SKINNED;
	pDesc->strModelPrototypeTag = pDesc->strModelPrototypeTag.empty() ? TEXT("Prototype_Component_Model_Monastery_WallDeco01") : pDesc->strModelPrototypeTag;
	pDesc->strAnimFolderTag = pDesc->strAnimFolderTag.empty() ? TEXT("Monastery_WallDeco01") : pDesc->strAnimFolderTag;
	pDesc->strAnimTag = pDesc->strAnimTag.empty() ? TEXT("Monastery_WallDeco01") : pDesc->strAnimTag;
	pDesc->eAnimTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	pDesc->bStaticAnimation = true;
	pDesc->bControllerEnabled = true;
	pDesc->bPlayDefault = false;
	pDesc->bApplyRootMotion = false;
	pDesc->bUsePhysics = false;
}

HRESULT CMapTrap::Ready_DynamicPhysics(const MAP_TRAP_DESC& Desc)
{
	if (nullptr != m_pDynamicRigidBody && nullptr != m_pBlockingRigidBody)
		return S_OK;

	if (nullptr != m_pDynamicRigidBody || nullptr != m_pBlockingRigidBody)
		return E_FAIL;

	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return E_FAIL;

	const _float4x4* pTrapBoneCombinedMatrix = m_pModelCom->Get_Bone_CombinedMatPtr("SD_WallTrap");

	if (nullptr == pTrapBoneCombinedMatrix)
		return E_FAIL;

	m_pDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pDynamicRigidBody)
		return E_FAIL;

	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;
	PhysicDesc.bIsKinematic = true;
	PhysicDesc.iFilterWord0 = Desc.iPhysicsFilterWord0;
	PhysicDesc.iFilterWord1 = Desc.iPhysicsFilterWord1;

	// 현재 오브젝트 Transform을 Dynamic Rigidbody 초기 Transform으로 사용한다.
	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	SHAPE_DESC ShapeDesc{};
	ShapeDesc.eType = GEOMETRY_TYPE::BOX;
	ShapeDesc.vHalfExtents = Desc.vPhysicsHalfExtents;
	ShapeDesc.vLocalOffset = Desc.vPhysicsLocalOffset;
	ShapeDesc.vLocalRot = Desc.vPhysicsLocalQuat;
	ShapeDesc.bTrigger = true;
	PhysicDesc.Shapes.push_back(ShapeDesc);

	// 피격 박스는 움직이는 SD_WallTrap Bone Pose를 Kinematic Target으로 사용한다.
	if (FAILED(m_pDynamicRigidBody->Initialize(&PhysicDesc)))
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	m_pDynamicRigidBody->Attach_ToBone(pTrapBoneCombinedMatrix, XMMatrixIdentity());
	m_pDynamicRigidBody->Physical_Update(0.f);

	// Idle에서는 피해 판정이 없어야 하므로 Active 진입 전까지 Scene에서 제외한다.
	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return E_FAIL;

	if (nullptr != pActor->getScene())
		m_pGameInstance->Remove_ActorFromScene(pActor);

	// 원통의 둥근 단면을 따르는 Capsule은 플레이어의 이동을 막는 Simulation Shape다.
	m_pBlockingRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pBlockingRigidBody)
		return E_FAIL;

	PHYSIC_DESC BlockingPhysicDesc{};
	BlockingPhysicDesc.pOwner = this;
	BlockingPhysicDesc.bIsKinematic = true;
	BlockingPhysicDesc.iFilterWord0 = ETOUI(FILTER_GROUP::OBJECT);
	BlockingPhysicDesc.iFilterWord1 = ETOUI(FILTER_GROUP::PLAYER);

	XMStoreFloat3(&BlockingPhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&BlockingPhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	SHAPE_DESC BlockingShapeDesc{};
	BlockingShapeDesc.eType = GEOMETRY_TYPE::CAPSULE;
	BlockingShapeDesc.fRadius = Desc.fBlockingRadius;
	BlockingShapeDesc.fHalfHeight = Desc.fBlockingHalfHeight;
	BlockingShapeDesc.vLocalOffset = Desc.vBlockingLocalOffset;
	// PhysX Capsule 기본 X축을 그대로 사용하고, Trigger가 아닌 Simulation Shape로 유지한다.
	BlockingShapeDesc.bTrigger = false;
	BlockingPhysicDesc.Shapes.push_back(BlockingShapeDesc);

	if (FAILED(m_pBlockingRigidBody->Initialize(&BlockingPhysicDesc)))
	{
		Safe_Release(m_pBlockingRigidBody);
		return E_FAIL;
	}

	m_pBlockingRigidBody->Attach_ToBone(pTrapBoneCombinedMatrix, XMMatrixIdentity());
	m_pBlockingRigidBody->Physical_Update(0.f);

	return S_OK;
}

HRESULT CMapTrap::Activate()
{
	if (TRAP_STATE::IDLE != m_eState)
		return S_OK;

	return Enter_Active();
}

void CMapTrap::Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition)
{
	__super::Set_MapTransform(vScale, vRotationDegree, vPosition);

	if (nullptr == m_pTransformCom)
		return;

	_vector vPhysicsPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vPhysicsRotation = m_pTransformCom->Get_Quat();

	if (nullptr != m_pDynamicRigidBody)
		m_pDynamicRigidBody->Set_GlobalTransform(vPhysicsPosition, vPhysicsRotation);

	if (nullptr != m_pBlockingRigidBody)
		m_pBlockingRigidBody->Set_GlobalTransform(vPhysicsPosition, vPhysicsRotation);
}

HRESULT CMapTrap::Enter_Idle()
{
	m_eState = TRAP_STATE::IDLE;
	m_bActiveAnimationEntered = false;
	m_bPhysicsActivatePending = false;
	m_bPhysicsDeactivatePending = true;
	m_HitTargets.clear();

	return S_OK;
}

HRESULT CMapTrap::Enter_Active()
{
	m_eState = TRAP_STATE::ACTIVE;
	m_bActiveAnimationEntered = false;
	m_fReactivateTimer = 0.f;
	m_HitTargets.clear();

	Request_AnimState(TEXT("WallTrap_Active"));

	return S_OK;
}

void CMapTrap::Update_MapGameplayObject(_float fTimeDelta)
{
	if (TRAP_STATE::IDLE == m_eState)
	{
		// Active 애니메이션 완주 후 1.5초가 지나고 플레이어가 남아 있을 때만 다시 발동한다.
		if (m_fReactivateTimer > 0.f)
			m_fReactivateTimer = max(0.f, m_fReactivateTimer - fTimeDelta);

		if (true == m_bPlayerInsideActivationTrigger && m_fReactivateTimer <= 0.f)
			Activate();

		return;
	}

	if (TRAP_STATE::ACTIVE != m_eState)
		return;

	if (true == Is_AnimInState(TEXT("WallTrap_Active")))
	{
		if (false == m_bActiveAnimationEntered)
		{
			m_bActiveAnimationEntered = true;
			m_bPhysicsActivatePending = true;
		}

		return;
	}

	// Controller가 Active 종료 후 자동으로 Idle에 복귀한 시점에 피해 판정도 함께 종료한다.
	if (true == m_bActiveAnimationEntered && true == Is_AnimInState(TEXT("WallTrap_Idle")))
	{
		Enter_Idle();
		m_fReactivateTimer = REACTIVATE_DELAY;
	}
}

void CMapTrap::Apply_PendingPhysicsState()
{
	if (nullptr == m_pDynamicRigidBody || nullptr == m_pGameInstance)
		return;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return;

	if (true == m_bPhysicsDeactivatePending)
	{
		if (nullptr != pActor->getScene())
			m_pGameInstance->Remove_ActorFromScene(pActor);

		m_bPhysicsDeactivatePending = false;
	}

	if (true == m_bPhysicsActivatePending)
	{
		if (nullptr == pActor->getScene())
			m_pGameInstance->Add_ActorToScene(pActor);

		m_bPhysicsActivatePending = false;
	}
}

void CMapTrap::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
	Apply_PendingPhysicsState();

	if (nullptr == m_pModelCom || nullptr == m_pAnimator)
		return;

	// Update에서 계산된 최신 Animation Pose를 물리 Simulation 전에 두 Kinematic Rigidbody에 반영한다.
	m_pModelCom->Update_Bones(&m_pAnimator->Get_CurPose());

	if (nullptr != m_pBlockingRigidBody && nullptr != m_pBlockingRigidBody->Get_RigidActor() && nullptr != m_pBlockingRigidBody->Get_RigidActor()->getScene())
		m_pBlockingRigidBody->Physical_Update(fFixedTime);

	if (nullptr != m_pDynamicRigidBody && nullptr != m_pDynamicRigidBody->Get_RigidActor() && nullptr != m_pDynamicRigidBody->Get_RigidActor()->getScene())
		m_pDynamicRigidBody->Physical_Update(fFixedTime);
}

void CMapTrap::Begin_Overlap(CGameObject* pOther)
{
	if (TRAP_STATE::ACTIVE != m_eState || false == m_bActiveAnimationEntered || nullptr == pOther || nullptr == pOther->Get_Transform())
		return;

	// 한 번의 Active 애니메이션에서는 같은 대상을 한 번만 피격한다.
	if (false == m_HitTargets.emplace(pOther).second)
		return;

	_vector vTrapPosition = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTrapQuaternion = m_pTransformCom->Get_Quat();

	if (nullptr != m_pDynamicRigidBody)
		m_pDynamicRigidBody->Get_GlobalTransform(vTrapPosition, vTrapQuaternion);

	const _vector vTargetPosition = pOther->Get_Transform()->Get_State(STATE::POSITION);
	_vector vHitNormal = XMVectorSetW(vTargetPosition - vTrapPosition, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vHitNormal)) <= FLT_EPSILON)
		vHitNormal = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	else
		vHitNormal = XMVector3Normalize(vHitNormal);

	// 피해 Trigger가 닿은 한 프레임에 CCT를 즉시 트랩 바깥 방향으로 이동시킨다.
	CPlayer* pPlayer = static_cast<CPlayer*>(pOther);
	pPlayer->Apply_ExternalDisplacement(vHitNormal * m_fPushDistance);

	HitInfo Hit{};
	Hit.pOtherActor = this;
	Hit.pDamangeCauser = this;
	Hit.fDamage = m_fDamage;
	Hit.eHitType = HIT_TYPE::KnockDown;

	// Player는 vHitNormal이 아니라 vHitPosition으로 피격 방향을 판정하므로 반대편 기준점을 전달한다.
	const _vector vReversedHitDirectionPosition = vTargetPosition + vHitNormal;
	XMStoreFloat3(&Hit.vHitPosition, vReversedHitDirectionPosition);
	XMStoreFloat3(&Hit.vHitNormal, vHitNormal);

	pOther->Take_Damage(Hit);
}

void CMapTrap::On_LinkedTriggerBegin(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// 외부 활성화 Trigger 안에 플레이어가 남아 있는 동안 반복 발동 조건을 유지한다.
	m_bPlayerInsideActivationTrigger = true;

	if (m_fReactivateTimer <= 0.f)
		Activate();
}

void CMapTrap::On_LinkedTriggerEnd(CGameObject* pInstigator)
{
	if (nullptr == pInstigator)
		return;

	// Trigger를 벗어나면 현재 애니메이션은 끝까지 재생하되 다음 반복만 중단한다.
	m_bPlayerInsideActivationTrigger = false;
}

void CMapTrap::On_LinkedActivate()
{
	Activate();
}

CMapTrap* CMapTrap::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMapTrap* pInstance = new CMapTrap(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMapTrap");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMapTrap::Clone(void* pArg)
{
	CMapTrap* pInstance = new CMapTrap(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapTrap");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapTrap::Free()
{
	m_HitTargets.clear();
	Safe_Release(m_pBlockingRigidBody);
	Safe_Release(m_pDynamicRigidBody);

	__super::Free();
}

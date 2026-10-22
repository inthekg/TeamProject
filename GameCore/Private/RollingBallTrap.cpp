#include "RollingBallTrap.h"

#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "Transform.h"

CRollingBallTrap::CRollingBallTrap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
}

CRollingBallTrap::CRollingBallTrap(const CRollingBallTrap& Prototype)
	: CMapGameplayObject{ Prototype }
	, m_eState{ BALL_STATE::END }
{
}

HRESULT CRollingBallTrap::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CRollingBallTrap::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	ROLLING_BALL_DESC* pDesc = static_cast<ROLLING_BALL_DESC*>(pArg);

	if (false == Is_ValidDesc(*pDesc))
		return E_FAIL;

	const _float fUniformScale = pDesc->vScale.x;

	// 공은 부모의 Static Rigidbody 대신 직접 생성한 Dynamic Rigidbody를 사용한다.
	MAP_GAMEPLAY_OBJECT_DESC BaseDesc = *pDesc;
	BaseDesc.eVisualType = VISUAL_TYPE::STATIC;
	BaseDesc.bUsePhysics = false;
	BaseDesc.bControllerEnabled = false;
	BaseDesc.bPlayDefault = false;
	BaseDesc.bApplyRootMotion = false;

	if (FAILED(__super::Initialize(&BaseDesc)))
		return E_FAIL;

	m_Waypoints = pDesc->Waypoints;
	m_vResetPosition = pDesc->vResetPosition;

	// Ball Radius는 Scale 1 기준이며 모델과 같은 균일 Scale을 물리에 적용한다.
	m_fRadius = pDesc->fRadius * fUniformScale;
	m_fTriggerRadius = pDesc->fTriggerRadius * fUniformScale;
	m_fMass = pDesc->fMass;

	m_fMoveSpeed = pDesc->fMoveSpeed;
	m_fSteerGain = pDesc->fSteerGain;
	m_fMaxSteerAcceleration = pDesc->fMaxSteerAcceleration;
	m_fLookAheadDistance = pDesc->fLookAheadDistance;

	m_fWaypointRadius = pDesc->fWaypointRadius;
	m_fFinishRadius = pDesc->fFinishRadius;
	m_fHiddenTime = pDesc->fHiddenTime;

	m_fDamage = pDesc->fDamage;

	m_fKillY = pDesc->fKillY;
	m_fStuckSpeed = pDesc->fStuckSpeed;
	m_fStuckTime = pDesc->fStuckTime;

	XMStoreFloat4(&m_vResetQuaternion, m_pTransformCom->Get_Quat());

	// Runtime과 Editor Preview 모두 요청된 경우에만 Dynamic Rigidbody를 생성한다.
	if (true == pDesc->bPhysicsStartRegistered && FAILED(Ready_DynamicPhysics(m_fRadius, m_fTriggerRadius, m_fMass)))
		return E_FAIL;

	m_iWaypointIndex = 0;
	m_fHiddenTimer = 0.f;
	m_fStuckTimer = 0.f;
	m_eState = BALL_STATE::ROLLING;

	Set_Visible(true);

	return S_OK;
}

_bool CRollingBallTrap::Is_ValidDesc(const ROLLING_BALL_DESC& Desc) const
{
	if (Desc.strModelPrototypeTag.empty() || Desc.Waypoints.empty())
		return false;

	if (false == std::isfinite(Desc.vResetPosition.x) || false == std::isfinite(Desc.vResetPosition.y) || false == std::isfinite(Desc.vResetPosition.z))
		return false;

	// PhysX Sphere와 모델이 같은 배율을 사용하도록 균일 Scale만 허용한다.
	if (false == std::isfinite(Desc.vScale.x) || false == std::isfinite(Desc.vScale.y) || false == std::isfinite(Desc.vScale.z))
		return false;

	if (Desc.vScale.x <= 0.f || Desc.vScale.y <= 0.f || Desc.vScale.z <= 0.f)
		return false;

	if (fabsf(Desc.vScale.x - Desc.vScale.y) > 0.0001f || fabsf(Desc.vScale.x - Desc.vScale.z) > 0.0001f)
		return false;

	if (false == std::isfinite(Desc.fRadius) || false == std::isfinite(Desc.fTriggerRadius) || false == std::isfinite(Desc.fMass))
		return false;

	if (Desc.fRadius <= 0.f || Desc.fTriggerRadius < Desc.fRadius || Desc.fMass <= 0.f)
		return false;

	if (false == std::isfinite(Desc.fMoveSpeed) || false == std::isfinite(Desc.fSteerGain) || false == std::isfinite(Desc.fMaxSteerAcceleration) || false == std::isfinite(Desc.fLookAheadDistance))
		return false;

	if (Desc.fMoveSpeed <= 0.f || Desc.fSteerGain <= 0.f || Desc.fMaxSteerAcceleration <= 0.f || Desc.fLookAheadDistance <= 0.f)
		return false;

	if (false == std::isfinite(Desc.fWaypointRadius) || false == std::isfinite(Desc.fFinishRadius) || false == std::isfinite(Desc.fHiddenTime))
		return false;

	if (Desc.fWaypointRadius <= 0.f || Desc.fFinishRadius <= 0.f || Desc.fHiddenTime < 0.f)
		return false;

	if (false == std::isfinite(Desc.fDamage) || false == std::isfinite(Desc.fKillY) || false == std::isfinite(Desc.fStuckSpeed) || false == std::isfinite(Desc.fStuckTime))
		return false;

	if (Desc.fDamage < 0.f || Desc.fStuckSpeed < 0.f || Desc.fStuckTime <= 0.f)
		return false;

	for (const _float3& vWaypoint : Desc.Waypoints)
	{
		if (false == std::isfinite(vWaypoint.x) || false == std::isfinite(vWaypoint.y) || false == std::isfinite(vWaypoint.z))
			return false;
	}

	return true;
}

HRESULT CRollingBallTrap::Ready_DynamicPhysics(_float fRadius, _float fTriggerRadius, _float fMass)
{
	if (nullptr != m_pDynamicRigidBody)
		return S_OK;

	m_pDynamicRigidBody = CDynamicRigidBody::Create(m_pDevice, m_pContext);

	if (nullptr == m_pDynamicRigidBody)
		return E_FAIL;

	PHYSIC_DESC PhysicDesc{};
	PhysicDesc.pOwner = this;
	PhysicDesc.bIsKinematic = false;
	PhysicDesc.fMass = fMass;

	// Shape별 필터는 Actor 초기화 직후 Configure_PhysicsShapes에서 다시 설정한다.
	PhysicDesc.iFilterWord0 = 0;
	PhysicDesc.iFilterWord1 = 0;

	XMStoreFloat3(&PhysicDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&PhysicDesc.vInitQuat, m_pTransformCom->Get_Quat());

	// 지형과 실제 물리 충돌을 담당하는 Sphere다.
	SHAPE_DESC SimulationShape{};
	SimulationShape.eType = GEOMETRY_TYPE::SPHERE;
	SimulationShape.fRadius = fRadius;
	SimulationShape.bTrigger = false;
	PhysicDesc.Shapes.push_back(SimulationShape);

	// 플레이어를 막지 않고 피격 이벤트만 받는 Sphere Trigger다.
	SHAPE_DESC PlayerTriggerShape{};
	PlayerTriggerShape.eType = GEOMETRY_TYPE::SPHERE;
	PlayerTriggerShape.fRadius = fTriggerRadius;
	PlayerTriggerShape.bTrigger = true;
	PhysicDesc.Shapes.push_back(PlayerTriggerShape);

	if (FAILED(m_pDynamicRigidBody->Initialize(&PhysicDesc)))
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	if (FAILED(Configure_PhysicsShapes()))
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
	{
		Safe_Release(m_pDynamicRigidBody);
		return E_FAIL;
	}

	PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(pActor);

	// 공이 너무 빨리 멈추지 않도록 감쇠를 낮게 유지한다.
	pDynamic->setLinearDamping(0.05f);
	pDynamic->setAngularDamping(0.05f);
	pDynamic->setMaxAngularVelocity(40.f);

	return S_OK;
}

HRESULT CRollingBallTrap::Configure_PhysicsShapes()
{
	if (nullptr == m_pDynamicRigidBody)
		return E_FAIL;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return E_FAIL;

	const PxU32 iShapeCount = pActor->getNbShapes();

	if (2 != iShapeCount)
		return E_FAIL;

	vector<PxShape*> Shapes(iShapeCount);
	pActor->getShapes(Shapes.data(), iShapeCount);

	// 기본 Material의 높은 반발력을 피하기 위해 공 전용 Material을 사용한다.
	PxMaterial* pBallMaterial = m_pGameInstance->Create_PxMaterial(0.8f, 0.6f, 0.05f);

	if (nullptr == pBallMaterial)
		return E_FAIL;

	_bool bSimulationShapeFound = false;
	_bool bTriggerShapeFound = false;

	for (PxShape* pShape : Shapes)
	{
		if (nullptr == pShape)
			continue;

		const PxShapeFlags ShapeFlags = pShape->getFlags();

		if (ShapeFlags.isSet(PxShapeFlag::eTRIGGER_SHAPE))
		{
			// Trigger가 PLAYER를 요청하므로 플레이어 쪽 필터를 바꾸지 않아도 감지된다.
			m_pDynamicRigidBody->Set_Filter(pShape, ETOUI(FILTER_GROUP::TRIGGER), ETOUI(FILTER_GROUP::PLAYER));
			bTriggerShapeFound = true;
			continue;
		}

		if (ShapeFlags.isSet(PxShapeFlag::eSIMULATION_SHAPE))
		{
			// word0을 0으로 둬 플레이어와 무기 쿼리 대상이 되지 않게 한다.
			// word1에서 지형 그룹을 요청하므로 GROUND 및 OBJECT와는 물리 충돌한다.
			m_pDynamicRigidBody->Set_Filter(pShape, 0, ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));

			PxMaterial* ShapeMaterials[1] = { pBallMaterial };
			pShape->setMaterials(ShapeMaterials, 1);

			bSimulationShapeFound = true;
		}
	}

	pBallMaterial->release();

	if (false == bSimulationShapeFound || false == bTriggerShapeFound)
		return E_FAIL;

	// Actor가 이미 Scene에 등록된 후 필터를 변경했으므로 필터 Pair를 다시 계산한다.
	m_pGameInstance->Reset_Filtering(pActor);

	return S_OK;
}

void CRollingBallTrap::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);

	if (nullptr == m_pDynamicRigidBody || nullptr == m_pTransformCom)
		return;

	if (BALL_STATE::HIDDEN == m_eState)
	{
		m_fHiddenTimer += fFixedTime;

		if (m_fHiddenTimer >= m_fHiddenTime)
			Reset_Cycle();

		return;
	}

	if (BALL_STATE::ROLLING != m_eState)
		return;

	// 이전 PhysX simulation 결과의 위치와 회전을 오브젝트 Transform에 반영한다.
	m_pDynamicRigidBody->Physical_Update(fFixedTime);

	const _vector vPosition = m_pTransformCom->Get_State(STATE::POSITION);

	if (XMVectorGetY(vPosition) <= m_fKillY)
	{
		Enter_Hidden();
		return;
	}

	Advance_Waypoint(vPosition);

	if (Has_ReachedFinish(vPosition))
	{
		Enter_Hidden();
		return;
	}

	Update_StuckState(fFixedTime);

	if (BALL_STATE::ROLLING != m_eState)
		return;

	// 다음 PhysX simulation에서 반영될 수평 유도력을 적용한다.
	Drive_Rolling();
}

void CRollingBallTrap::Advance_Waypoint(_fvector vPosition)
{
	if (m_Waypoints.empty())
		return;

	while (m_iWaypointIndex + 1 < static_cast<_uint>(m_Waypoints.size()))
	{
		_vector vWaypoint = XMVectorSetW(XMLoadFloat3(&m_Waypoints[m_iWaypointIndex]), 1.f);
		_vector vToWaypoint = XMVectorSetW(XMVectorSetY(vWaypoint - vPosition, 0.f), 0.f);

		const _float fDistanceSq = XMVectorGetX(XMVector3LengthSq(vToWaypoint));

		if (fDistanceSq > m_fWaypointRadius * m_fWaypointRadius)
			break;

		++m_iWaypointIndex;
	}
}

_bool CRollingBallTrap::Has_ReachedFinish(_fvector vPosition) const
{
	if (m_Waypoints.empty() || m_iWaypointIndex + 1 != static_cast<_uint>(m_Waypoints.size()))
		return false;

	_vector vFinish = XMVectorSetW(XMLoadFloat3(&m_Waypoints.back()), 1.f);
	_vector vToFinish = XMVectorSetW(vFinish - vPosition, 0.f);

	return XMVectorGetX(XMVector3LengthSq(vToFinish)) <= m_fFinishRadius * m_fFinishRadius;
}

_vector CRollingBallTrap::Compute_GuidePoint(_fvector vPosition) const
{
	_vector vCurrentTarget = XMVectorSetW(XMLoadFloat3(&m_Waypoints[m_iWaypointIndex]), 1.f);

	if (m_iWaypointIndex + 1 >= static_cast<_uint>(m_Waypoints.size()))
		return vCurrentTarget;

	_vector vToCurrentTarget = XMVectorSetW(XMVectorSetY(vCurrentTarget - vPosition, 0.f), 0.f);
	const _float fDistance = XMVectorGetX(XMVector3Length(vToCurrentTarget));

	if (fDistance >= m_fLookAheadDistance)
		return vCurrentTarget;

	const _float fBlend = 1.f - fDistance / m_fLookAheadDistance;
	_vector vNextTarget = XMVectorSetW(XMLoadFloat3(&m_Waypoints[m_iWaypointIndex + 1]), 1.f);

	// 현재 목표에 가까워질수록 다음 목표 쪽으로 미리 방향을 전환한다.
	return XMVectorLerp(vCurrentTarget, vNextTarget, fBlend);
}

void CRollingBallTrap::Drive_Rolling()
{
	if (nullptr == m_pDynamicRigidBody || m_Waypoints.empty())
		return;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return;

	PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(pActor);

	const _vector vPosition = m_pTransformCom->Get_State(STATE::POSITION);
	const _vector vGuidePoint = Compute_GuidePoint(vPosition);

	_vector vToGuide = XMVectorSetW(XMVectorSetY(vGuidePoint - vPosition, 0.f), 0.f);
	const _float fToGuideLengthSq = XMVectorGetX(XMVector3LengthSq(vToGuide));

	if (fToGuideLengthSq <= FLT_EPSILON)
		return;

	const _vector vDesiredDirection = XMVector3Normalize(vToGuide);
	const _vector vDesiredVelocity = vDesiredDirection * m_fMoveSpeed;

	const PxVec3 CurrentPxVelocity = pDynamic->getLinearVelocity();
	_vector vCurrentVelocity = XMVectorSet(CurrentPxVelocity.x, 0.f, CurrentPxVelocity.z, 0.f);

	_vector vAcceleration = (vDesiredVelocity - vCurrentVelocity) * m_fSteerGain;
	const _float fAccelerationLengthSq = XMVectorGetX(XMVector3LengthSq(vAcceleration));
	const _float fMaxAccelerationSq = m_fMaxSteerAcceleration * m_fMaxSteerAcceleration;

	if (fAccelerationLengthSq > fMaxAccelerationSq)
		vAcceleration = XMVector3Normalize(vAcceleration) * m_fMaxSteerAcceleration;

	// Y축은 건드리지 않으므로 중력과 계단 낙하가 그대로 유지된다.
	m_pDynamicRigidBody->Add_Force(vAcceleration, PxForceMode::eACCELERATION);
}

void CRollingBallTrap::Update_StuckState(_float fFixedTime)
{
	if (nullptr == m_pDynamicRigidBody || m_Waypoints.empty())
		return;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return;

	PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(pActor);
	const PxVec3 CurrentVelocity = pDynamic->getLinearVelocity();

	const _float fSpeedSq = CurrentVelocity.magnitudeSquared();
	const _float fStuckSpeedSq = m_fStuckSpeed * m_fStuckSpeed;

	if (fSpeedSq <= fStuckSpeedSq)
		m_fStuckTimer += fFixedTime;
	else
		m_fStuckTimer = 0.f;

	if (m_fStuckTimer >= m_fStuckTime)
		Enter_Hidden();
}

void CRollingBallTrap::Begin_Overlap(CGameObject* pOther)
{
	if (BALL_STATE::ROLLING != m_eState || nullptr == pOther || nullptr == pOther->Get_Transform())
		return;

	// 한 번의 경로 순환에서는 같은 대상을 한 번만 피격한다.
	if (false == m_HitTargets.emplace(pOther).second)
		return;

	const _vector vBallPosition = m_pTransformCom->Get_State(STATE::POSITION);
	const _vector vTargetPosition = pOther->Get_Transform()->Get_State(STATE::POSITION);

	_vector vHitNormal = XMVectorSetW(vTargetPosition - vBallPosition, 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vHitNormal)) <= FLT_EPSILON)
		vHitNormal = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	else
		vHitNormal = XMVector3Normalize(vHitNormal);

	const _vector vHitPosition = vBallPosition + vHitNormal * m_fRadius;

	HitInfo Hit{};
	Hit.pOtherActor = this;
	Hit.pDamangeCauser = this;
	Hit.fDamage = m_fDamage;
	Hit.eHitType = HIT_TYPE::KnockDown;

	XMStoreFloat3(&Hit.vHitPosition, vHitPosition);
	XMStoreFloat3(&Hit.vHitNormal, vHitNormal);

	// 플레이어 타입으로 캐스팅하지 않고 공통 피격 인터페이스만 사용한다.
	pOther->Take_Damage(Hit);
}

void CRollingBallTrap::End_Overlap(CGameObject* pOther)
{
	// 같은 경로 순환 중 재진입 타격을 막기 위해 여기서는 HitTargets를 제거하지 않는다.
}

void CRollingBallTrap::Enter_Hidden()
{
	if (BALL_STATE::HIDDEN == m_eState)
		return;

	m_eState = BALL_STATE::HIDDEN;
	m_fHiddenTimer = 0.f;
	m_fStuckTimer = 0.f;

	Set_Visible(false);

	PxRigidActor* pActor = nullptr;

	if (nullptr != m_pDynamicRigidBody)
		pActor = m_pDynamicRigidBody->Get_RigidActor();

	// Scene 변경은 PhysX Trigger 콜백이 아니라 고정 업데이트에서 수행한다.
	if (nullptr != pActor && nullptr != pActor->getScene())
		m_pGameInstance->Remove_ActorFromScene(pActor);

	Reset_PhysicsPose();
}

void CRollingBallTrap::Reset_PhysicsPose()
{
	if (nullptr == m_pDynamicRigidBody || nullptr == m_pTransformCom)
		return;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return;

	PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(pActor);

	const _vector vResetPosition = XMVectorSetW(XMLoadFloat3(&m_vResetPosition), 1.f);
	const _vector vResetQuaternion = XMQuaternionNormalize(XMLoadFloat4(&m_vResetQuaternion));

	// 속도와 Transform은 Scene 밖에서도 초기화할 수 있다.
	pDynamic->setLinearVelocity(PxVec3(0.f, 0.f, 0.f));
	pDynamic->setAngularVelocity(PxVec3(0.f, 0.f, 0.f));

	// clearForce와 clearTorque는 Actor가 PhysX Scene에 들어가 있을 때만 호출할 수 있다.
	if (nullptr != pActor->getScene())
	{
		pDynamic->clearForce(PxForceMode::eFORCE);
		pDynamic->clearTorque(PxForceMode::eFORCE);
	}

	m_pDynamicRigidBody->Set_GlobalTransform(vResetPosition, vResetQuaternion);

	m_pTransformCom->Set_State(STATE::POSITION, vResetPosition);
	m_pTransformCom->Set_Quat(vResetQuaternion);

	m_iWaypointIndex = 0;
	m_fStuckTimer = 0.f;
	m_HitTargets.clear();
}
void CRollingBallTrap::Reset_Cycle()
{
	if (nullptr == m_pDynamicRigidBody)
		return;

	PxRigidActor* pActor = m_pDynamicRigidBody->Get_RigidActor();

	if (nullptr == pActor)
		return;

	Reset_PhysicsPose();

	if (nullptr == pActor->getScene())
		m_pGameInstance->Add_ActorToScene(pActor);

	PxRigidDynamic* pDynamic = static_cast<PxRigidDynamic*>(pActor);
	pDynamic->wakeUp();

	m_fHiddenTimer = 0.f;
	m_eState = BALL_STATE::ROLLING;

	Set_Visible(true);
}

void CRollingBallTrap::Set_Visible(_bool bVisible)
{
	for (_uint i = 0; i < static_cast<_uint>(m_MeshVisibles.size()); ++i)
		m_MeshVisibles[i] = bVisible;
}

CRollingBallTrap* CRollingBallTrap::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CRollingBallTrap* pInstance = new CRollingBallTrap(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CRollingBallTrap");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CRollingBallTrap::Clone(void* pArg)
{
	CRollingBallTrap* pInstance = new CRollingBallTrap(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CRollingBallTrap");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CRollingBallTrap::Free()
{
	Safe_Release(m_pDynamicRigidBody);

	m_Waypoints.clear();
	m_HitTargets.clear();

	__super::Free();
}

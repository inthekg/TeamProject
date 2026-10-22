#include "Movement_AI.h"
#include "Transform.h"
#include "Navigation.h"
#include "Engine_Utility.h"
#include "AIObject.h"

static const float NAVI_REPATH_GOAL_MOVE = 2.f;
static const float NAVI_WAYPOINT_MARGIN = 0.8f;
static const float NAVI_PATH_HOLD = 0.8f;

CMovement_AI::CMovement_AI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CMovement_AI::CMovement_AI(const CMovement_AI& Prototype)
	: CComponent{ Prototype }
{
}

void CMovement_AI::Set_MoveDir(_fvector vWorldDir)
{
	_vector vFlat = XMVectorSetY(vWorldDir, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vFlat)) > 1e-6f)
	{
		XMStoreFloat4(&m_vMoveDir, XMVector3Normalize(vFlat));
		m_bHasSteerInput = true;
	}
	else
	{
		XMStoreFloat4(&m_vMoveDir, XMVectorZero());
		m_bHasSteerInput = false;
	}
}

void CMovement_AI::Set_MoveTarget(_fvector vWorldPos)
{
	if (nullptr == m_pOwnerTransformCom)
	{
		MSG_BOX("Movement_AI : Null Bind Owner Transform");
		return;
	}

	_vector vMyPos = m_pOwnerTransformCom->Get_State(STATE::POSITION);
	Set_MoveDir(XMVectorSubtract(vWorldPos, vMyPos));
}

_float CMovement_AI::Get_CurrentSpeed() const
{
	if (ETOUI(m_eSpeedType) >= ETOUI(SPEED_TYPE::END))
		return 0.f;
	return m_Speed[ETOUI(m_eSpeedType)];
}

_float CMovement_AI::Get_FallGravity() const
{
	const _float fWeight = (nullptr != m_pOwner) ? m_pOwner->Get_GravityWeight() : 1.f;
	return PhysX::g_fGravity * PhysX::g_fGravityScale * fWeight;
}

_bool CMovement_AI::Is_Reached(_fvector vWorldPos, _float fToL) const
{
	if (nullptr == m_pOwnerTransformCom)
		return false;
	_vector vMyPos = m_pOwnerTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSubtract(XMVectorSetY(vWorldPos, XMVectorGetY(vMyPos)), vMyPos);
	return XMVectorGetX(XMVector3Length(vTo)) <= fToL;
}

HRESULT CMovement_AI::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CMovement_AI::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Nullptr Argument : Movement_AI");
		return E_FAIL;
	}

	MOVEMENT_DESC* pDesc = static_cast<MOVEMENT_DESC*>(pArg);

	m_pOwner = pDesc->pOwner;
	m_pOwnerTransformCom = (nullptr != pDesc->pTransform) ? pDesc->pTransform
		: (nullptr != m_pOwner) ? m_pOwner->Get_Transform() : nullptr;
	if (nullptr == m_pOwnerTransformCom)
	{
		MSG_BOX("Nullptr Transform : Movement_AI");
		return E_FAIL;
	}

	m_pNavigation = pDesc->pNavigation;
	m_Speed[ETOUI(SPEED_TYPE::RUN)] = pDesc->fRunSpeed;
	m_Speed[ETOUI(SPEED_TYPE::SPRINT)] = pDesc->fSprintSpeed;

	m_fAcceleration = pDesc->fAcceleration;
	m_fDeceleration = pDesc->fDeceleration;
	m_fGroundStick = pDesc->fGroundStick;
	m_fNaviRepathInterval = (pDesc->fNaviRepathInterval > 0.f) ? pDesc->fNaviRepathInterval : 0.3f;
	m_fRotationPerSec = (pDesc->fRotationPerSec > 0.f) ? pDesc->fRotationPerSec : m_pOwnerTransformCom->Get_RotationPerSec();
	m_fAgentRadius = pDesc->fAgentRadius * 0.9f;
	m_fCornerInset = pDesc->fAgentRadius * 4.f;

	return S_OK;
}

CMovement_AI::MOVEMENT_RESULT CMovement_AI::Compute_Movement(_float fTimeDelta)
{
	m_tMovementResult = {};
	_vector vMoveDir = XMLoadFloat4(&m_vMoveDir);

	if (true == m_bLaunching)
	{
		m_tMovementResult.vMoveDir.x = m_vLaunchVelXZ.x * fTimeDelta;
		m_tMovementResult.vMoveDir.z = m_vLaunchVelXZ.z * fTimeDelta;

		if (nullptr != m_pNavigation)
		{
			_float3 vTarget = {};
			XMStoreFloat3(&vTarget, m_pOwnerTransformCom->Get_State(STATE::POSITION));
			vTarget.x += m_tMovementResult.vMoveDir.x;
			vTarget.z += m_tMovementResult.vMoveDir.z;

			_float3 vOut = {};
			if (false == m_pNavigation->Move_OnNavigation(vTarget, &m_iNaviCurCellIndex, &vOut))
			{
				m_tMovementResult.vMoveDir.x = 0.f;
				m_tMovementResult.vMoveDir.z = 0.f;
				m_vLaunchVelXZ = {};
			}
		}

		m_fFallVelocityY += Get_FallGravity() * fTimeDelta;
		if (m_fFallVelocityY < MAX_FALL_SPEED)
			m_fFallVelocityY = MAX_FALL_SPEED;
		m_tMovementResult.vMoveDir.y = m_fFallVelocityY * fTimeDelta;

		if (true == m_bGrounded && m_fFallVelocityY < 0.f)
		{
			m_bLaunching = false;
			m_vLaunchVelXZ = {};
		}

		m_vVelocity = { 0.f, 0.f, 0.f };
		m_bHasSteerInput = false;
		return m_tMovementResult;
	}

	if (m_bMoveTo)
	{
		m_fMoveElapsedTime += fTimeDelta;
		_float fTime = min(m_fMoveElapsedTime / m_fMoveDuration, 1.f);
		_float fEaseValue = Ease(m_eMoveEaseType, fTime);

		_vector vNextPos = XMVectorLerp(XMLoadFloat4(&m_vMoveStartPos), XMLoadFloat4(&m_vMoveTargetPos), fEaseValue);
		_vector vDeltaPos = vNextPos - m_pOwnerTransformCom->Get_State(STATE::POSITION);

		m_tMovementResult.vMoveDir = _float3(XMVectorGetX(vDeltaPos), 0.f, XMVectorGetZ(vDeltaPos));
		m_vVelocity = { 0.f, 0.f, 0.f };

		_vector vTargetQuat = XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&m_vMoveStartQuat), XMLoadFloat4(&m_vMoveTargetQuat), fEaseValue));
		m_tMovementResult.bChangedQuat = true;
		XMStoreFloat4(&m_tMovementResult.vMoveQuat, vTargetQuat);

		if (fTime >= 1.f || m_bSideBlocked)
		{
			m_bMoveTo = false;
			m_bMoveReached = true;
		}
	}
	else if (m_bPlayRootMotion)
	{
		_float fClampRotationPerSec = min(m_fRotationPerSec * fTimeDelta, 1.f);
		_vector vRefQuat = XMQuaternionSlerp(m_pOwnerTransformCom->Get_Quat(), XMLoadFloat4(&m_vRootMotionQuat), fClampRotationPerSec);

		_vector vLocalDelta = XMLoadFloat4(&m_tRootMotionTransform.vTranslation);
		_vector vWorldDelta = XMVector3Rotate(vLocalDelta, vRefQuat);

		m_tMovementResult.vMoveDir = _float3(XMVectorGetX(vWorldDelta), XMVectorGetY(vWorldDelta), XMVectorGetZ(vWorldDelta));
		if (true == m_bHasSteerInput)
		{
			_float fHomeSpeed = (m_fHomingSpeed > 0.f) ? m_fHomingSpeed : Get_CurrentSpeed();
			_vector vHoming = fHomeSpeed * XMLoadFloat4(&m_vMoveDir) * fTimeDelta;
			m_tMovementResult.vMoveDir.x += XMVectorGetX(vHoming);
			m_tMovementResult.vMoveDir.z += XMVectorGetZ(vHoming);
		}
		m_vVelocity = { 0.f, 0.f, 0.f };
	}
	else
	{
		_float fMoveSpeed = Get_CurrentSpeed();
		_vector vTargetVelocity = (true == m_bHasSteerInput) ? fMoveSpeed * vMoveDir : XMVectorZero();
		_vector vVelocity;

		if (true == m_bUseAccel)
		{
			vVelocity = XMLoadFloat3(&m_vVelocity);
			if (true == m_bHasSteerInput)
			{
				_float fReverseDot = XMVectorGetX(XMVector3Dot(vMoveDir, vVelocity));
				_float fAccel = (fReverseDot < 0.f) ? m_fAcceleration * REVERSE_MULT_ACCEL : m_fAcceleration;
				_float fAccelAlpha = 1.f - expf(-fAccel * fTimeDelta);
				vVelocity = XMVectorLerp(vVelocity, vTargetVelocity, fAccelAlpha);
			}
			else
			{
				_float fDecelAlpha = 1.f - expf(-m_fDeceleration * fTimeDelta);
				vVelocity = XMVectorLerp(vVelocity, XMVectorZero(), fDecelAlpha);
				if (XMVectorGetX(XMVector3Length(vVelocity)) < 0.01f)
					vVelocity = XMVectorZero();
			}
		}
		else
		{
			vVelocity = vTargetVelocity;
		}

		XMStoreFloat3(&m_vVelocity, vVelocity);
		_vector vDeltaVelocity = vVelocity * fTimeDelta;
		m_tMovementResult.vMoveDir = _float3(XMVectorGetX(vDeltaVelocity), 0.f, XMVectorGetZ(vDeltaVelocity));
	}

	if (true == m_bGrounded)
	{
		m_fFallVelocityY = m_fGroundStick;
	}
	else
	{
		m_fFallVelocityY += Get_FallGravity() * fTimeDelta;
		if (m_fFallVelocityY < MAX_FALL_SPEED)
			m_fFallVelocityY = MAX_FALL_SPEED;
	}
	m_tMovementResult.vMoveDir.y += m_fFallVelocityY * fTimeDelta;

	m_bHasSteerInput = false;
	m_fHomingSpeed = 0.f;

	return m_tMovementResult;
}

_bool CMovement_AI::MoveTo_Navi(_fvector vWorldPos, _float fTimeDelta)
{
	if (nullptr == m_pNavigation || nullptr == m_pOwnerTransformCom)
	{
		m_NaviPath.clear();
		TurnTo(vWorldPos, fTimeDelta);
		Set_MoveTarget(vWorldPos);
		return true;
	}

	_float3 vGoal = {};
	XMStoreFloat3(&vGoal, vWorldPos);

	m_fNaviRepathAcc += fTimeDelta;

	if (true == m_bSideBlocked)
		m_fNaviRepathAcc = m_fNaviRepathInterval;

	_float fGoalDX = vGoal.x - m_vLastNaviGoal.x;
	_float fGoalDZ = vGoal.z - m_vLastNaviGoal.z;
	_bool bGoalMoved = (fGoalDX * fGoalDX + fGoalDZ * fGoalDZ) > (NAVI_REPATH_GOAL_MOVE * NAVI_REPATH_GOAL_MOVE);

	if (true == m_NaviPath.empty() || m_fNaviRepathAcc >= m_fNaviRepathInterval || true == bGoalMoved)
	{
		_float3 vStart = {};
		XMStoreFloat3(&vStart, m_pOwnerTransformCom->Get_State(STATE::POSITION));

		vector<_float3> NewPath = {};

		if (true == m_pNavigation->Find_Path(vStart, vGoal, &NewPath, m_fCornerInset) && NewPath.size() >= 2)
		{
			m_NaviPath = NewPath;
			m_iNaviPathIndex = 1;
			m_vLastNaviGoal = vGoal;
		}

		m_fNaviRepathAcc = 0.f;
	}

	if (true == m_NaviPath.empty())
	{
		TurnTo(vWorldPos, fTimeDelta);
		Set_MoveTarget(vWorldPos);
		return true;
	}

	_float fReach = m_fAgentRadius + NAVI_WAYPOINT_MARGIN;

	while (m_iNaviPathIndex + 1 < m_NaviPath.size()
		&& true == Is_Reached(XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]), fReach))
		++m_iNaviPathIndex;

	_vector vWaypoint = XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]);
	TurnTo(vWaypoint, fTimeDelta);
	Set_MoveTarget(vWaypoint);
	return true;
}



void CMovement_AI::TurnTo(_fvector vWorldPos, _float fTimeDelta)
{
	if (nullptr == m_pOwnerTransformCom)
		return;
	_vector vMyPos = m_pOwnerTransformCom->Get_State(STATE::POSITION);
	m_pOwnerTransformCom->LookAt(XMVectorSetY(vWorldPos, XMVectorGetY(vMyPos)), fTimeDelta);
}

void CMovement_AI::Strafe(_int iDir, _float fTimeDelta)
{
	if (nullptr == m_pOwnerTransformCom)
		return;

	_vector vAxis = XMVectorZero();
	switch (iDir)
	{
	case 0: vAxis = XMVectorNegate(m_pOwnerTransformCom->Get_State(STATE::RIGHT));	break;
	case 1: vAxis = m_pOwnerTransformCom->Get_State(STATE::RIGHT);					break;
	case 2: vAxis = XMVectorNegate(m_pOwnerTransformCom->Get_State(STATE::LOOK));	break;
	default: return;
	}
	Set_MoveDir(vAxis);
}

void CMovement_AI::Orbit(_int iDir, _fvector vTargetPos, _float fTimeDelta)
{
	if (nullptr == m_pOwnerTransformCom)
		return;

	_vector vMyPos = m_pOwnerTransformCom->Get_State(STATE::POSITION);
	_vector vToTarget = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vToTarget)) < 1e-6f)
		return;

	vToTarget = XMVector3Normalize(vToTarget);

	_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	_vector vTangent = XMVector3Normalize(XMVector3Cross(vUp, vToTarget));

	_vector vAxis = XMVectorZero();
	switch (iDir)
	{
	case 0: vAxis = XMVectorNegate(vTangent);	break;
	case 1: vAxis = vTangent;					break;
	case 2: vAxis = XMVectorNegate(vToTarget);	break;
	default: return;
	}

	Set_MoveDir(vAxis);
}

void CMovement_AI::Push_RootMotion(_bool bHasRootMotion, const TransformData& tDelta)
{
	if (bHasRootMotion)
	{
		m_tRootMotionTransform = tDelta;
		if (false == m_bPlayRootMotion)
		{
			m_bPlayRootMotion = true;
			XMStoreFloat4(&m_vRootMotionQuat, m_pOwnerTransformCom->Get_Quat());
		}
	}
	else
	{
		m_bPlayRootMotion = false;
	}
}

void CMovement_AI::Begin_MoveTo(_fvector vTargetPos, _fvector vTargetQuat, _float fDuration, EASE_TYPE eEaseType)
{
	XMStoreFloat4(&m_vMoveStartPos, m_pOwnerTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&m_vMoveStartQuat, m_pOwnerTransformCom->Get_Quat());

	XMStoreFloat4(&m_vMoveTargetPos, vTargetPos);
	XMStoreFloat4(&m_vMoveTargetQuat, XMQuaternionNormalize(vTargetQuat));

	m_bSideBlocked = false;
	m_fMoveDuration = (fDuration <= 0.f) ? 0.01f : fDuration;
	m_fMoveElapsedTime = 0.f;
	m_eMoveEaseType = eEaseType;
	m_bMoveReached = false;
	m_bMoveTo = true;
}

void CMovement_AI::Begin_KnockBack(_fvector vDirection, _float fDistance, _float fDuration, EASE_TYPE eEaseType)
{
	_vector vDir = XMVector3Normalize(XMVectorSetY(vDirection, 0.f));
	_vector vTargetPos = m_pOwnerTransformCom->Get_State(STATE::POSITION) + vDir * fDistance;
	Begin_MoveTo(vTargetPos, m_pOwnerTransformCom->Get_Quat(), fDuration, eEaseType);
}

void CMovement_AI::Stop_MoveTo()
{
	m_bMoveTo = false;
}

void CMovement_AI::Reset_Motion()
{
	m_bMoveTo = false;
	m_bLaunching = false;
	m_bGrounded = false;
	m_bPlayRootMotion = false;

	m_vVelocity = {};
	m_vLaunchVelXZ = {};
	m_fFallVelocityY = 0.f;

	m_NaviPath.clear();
	m_iNaviPathIndex = 0;
	m_fNaviRepathAcc = 0.f;
}


void CMovement_AI::Begin_Launch(_fvector vDir, _float fHorizontalSpeed, _float fUpSpeed)
{
	_vector vFlat = XMVector3Normalize(XMVectorSetY(vDir, 0.f));
	XMStoreFloat3(&m_vLaunchVelXZ, vFlat * fHorizontalSpeed);
	m_fFallVelocityY = fUpSpeed;

	if (nullptr != m_pNavigation && nullptr != m_pOwnerTransformCom)
	{
		_float3 vPos = {};
		XMStoreFloat3(&vPos, m_pOwnerTransformCom->Get_State(STATE::POSITION));
		m_pNavigation->Set_CurrentCell(vPos, &m_iNaviCurCellIndex);
	}

	m_bLaunching = true;
}

CMovement_AI* CMovement_AI::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMovement_AI* pInstance = new CMovement_AI(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMovement_AI");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CMovement_AI::Clone(void* pArg)
{
	CMovement_AI* pInstance = new CMovement_AI(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMovement_AI");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMovement_AI::Free()
{
	__super::Free();
}
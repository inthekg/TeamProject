#include "Movement.h"
#include "Player.h"
#include "Transform.h"
#include "InputComponent.h"
#include "Gameinstance.h"
#include "Status.h"
#include "Engine_Utility.h"
#include "AIObject.h"

CMovement::CMovement(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CComponent{ pDevice, pContext }
{
}

CMovement::CMovement(const CMovement& Prototype)
	:CComponent{ Prototype }
{
}

HRESULT CMovement::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CMovement::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CMovement : Argument is nullptr");
		return E_FAIL;
	}

	MOVEMENT_DESC* pDesc = static_cast<MOVEMENT_DESC*>(pArg);
	m_pOwner = pDesc->pOwner;
	if (m_pOwner == nullptr)
	{
		MSG_BOX("CMovement : Invalid Owner");
		return E_FAIL;
	}

	m_pOwnerTransformCom = m_pOwner->Get_Transform();
	m_pOwnerInputCom = m_pOwner->Get_InputComponent();
	m_pOwnerStatusCom = m_pOwner->Get_StatusComponent();

	if (m_pOwnerTransformCom == nullptr || m_pOwnerInputCom == nullptr || m_pOwnerStatusCom == nullptr)
	{
		MSG_BOX("CMovement : Invalid Owner Component");
		return E_FAIL;
	}

	m_Speed[ETOUI(SPEED_TYPE::RUN)] = pDesc->fRunSpeed;
	m_Speed[ETOUI(SPEED_TYPE::SPRINT)] = pDesc->fSprintSpeed;

	m_fAcceleration = pDesc->fAcceleration;
	m_fDeceleration = pDesc->fDeceleration;
	m_fTurnRate = pDesc->fTurnRate;

	return S_OK;
}

//입력축을 카메라 기준 방향으로 바꾸고 달리기/질주 판정
void CMovement::Update_Input()
{
	const _float fRawX = static_cast<_float>(m_pOwnerInputCom->Get_RawAxisX());
	const _float fRawY = static_cast<_float>(m_pOwnerInputCom->Get_RawAxisY());

	_vector vRightAxis = fRawX * XMVectorSetY(XMLoadFloat4(m_pGameInstance->Get_CamRight()), 0.f);
	_vector vLookAxis = fRawY * XMVectorSetY(XMLoadFloat4(m_pGameInstance->Get_CamLook()), 0.f);
	_vector vCamInput = vRightAxis + vLookAxis;

	//길이가 MIN_INPUT_LENGTH 보다 작으면 입력이 없다고 판단함
	m_bHasMoveInput = XMVectorGetX(XMVector3Length(vCamInput)) > MIN_INPUT_LENGTH;
	XMStoreFloat4(&m_vCamInputDir, m_bHasMoveInput ? XMVector3Normalize(vCamInput) : XMVectorZero());

	if (m_bHasMoveInput == false) m_eSpeedType = SPEED_TYPE::IDLE;
	//방향키 입력 + 스프린트 입력 모두 유효하면 스프린트
	else if (m_pOwnerInputCom->IsHold(PlayerActionID::Run)) m_eSpeedType = SPEED_TYPE::SPRINT;
	else m_eSpeedType = SPEED_TYPE::RUN;
}

void CMovement::Update_RootMotion(_bool bHasRootMotion, const TransformData& DeltaTransform)
{
	//새 액션 재시작 요청은 이번 프레임에만 유효
	const _bool bRestart = m_bRootMotionRestart;
	m_bRootMotionRestart = false;

	if (bHasRootMotion == false)
	{
		//루트모션이 끝나면 노티파이로 바꿨던 배율도 원래대로
		if (m_bPlayRootMotion)
			m_fRootMotionScale = 1.f;

		m_bPlayRootMotion = false;
		return;
	}

	m_tRootMotionTransform = DeltaTransform;

	//루트모션이 새로 켜졌거나, 이어서 플레이 되면 
	if (m_bPlayRootMotion == false || bRestart)
	{
		m_bPlayRootMotion = true;
		m_bRootMotionStarted = true;
		//루트모션의 시작 회전 고정
		XMStoreFloat4(&m_vRootMotionQuat, m_pOwnerTransformCom->Get_Quat());
	}
}

_float CMovement::Get_CurrentSpeed() const
{
	_float fSpeed = m_Speed[ETOUI(m_eSpeedType)];
	// 가드 중 속도 감소 
	if (m_pOwnerStatusCom->Is_GuardActive()) fSpeed *= GUARD_MOVE_RATE;
	return fSpeed;
}

_float CMovement::Get_FallGravity() const
{
	return CHARACTER_GRAVITY * m_pOwner->Get_GravityWeight();
}

CMovement::MOVEMENT_RESULT CMovement::Compute_Movement(_float fTimeDelta)
{
	m_tMovementResult = {};
	Update_Input();

	const _bool bGrounded = m_pOwnerStatusCom->Is_Grounded();
	const _bool bGravityDisabled = m_pOwnerStatusCom->Is_GravityDisabled();

	Handle_RootMotionStart();
	Update_AirState(fTimeDelta, bGrounded, bGravityDisabled);

	if (m_bMoveTo)
		Compute_MoveTo(fTimeDelta);
	else if (m_bPlayRootMotion)
		Compute_RootMotion(fTimeDelta);
	else if (m_bAirActive)
		Compute_AirControl(fTimeDelta);
	else
	{
		Compute_GroundMove(fTimeDelta);
		Compute_Facing(fTimeDelta);
	}

	//중력은 모든 경로 공통
	Apply_Gravity(fTimeDelta, bGrounded, bGravityDisabled);

	//공중에 뜨는 순간 이어받을 수평 속도
	if (bGrounded && fTimeDelta > 0.f)
		m_vLastGroundedXZ = _float2(m_tMovementResult.vMoveDelta.x / fTimeDelta, m_tMovementResult.vMoveDelta.z / fTimeDelta);

	return m_tMovementResult;
}

//루트모션 시작한 프레임에서만 값 세팅함
//입력이 있다면 그 방향을 루트모션 회전값으로 설정함 
void CMovement::Handle_RootMotionStart()
{
	if (m_bRootMotionStarted == false) return;
	m_bRootMotionStarted = false;

	if (m_pOwnerStatusCom->Is_LockedOn() || m_bHasMoveInput == false) return;

	_vector vTargetQuat = Get_Yaw_Quat(XMLoadFloat4(&m_vCamInputDir));

	XMStoreFloat4(&m_vRootMotionQuat, vTargetQuat);
	m_tMovementResult.bChangedQuat = true;
	XMStoreFloat4(&m_tMovementResult.vMoveQuat, vTargetQuat);
}

void CMovement::Update_AirState(_float fTimeDelta, _bool bGrounded, _bool bGravityDisabled)
{
	if (bGravityDisabled)
	{
		m_fAirTime = 0.f;
		m_bAirActive = false;
	}
	else if (bGrounded)
	{
		m_fAirTime = 0.f;
		if (m_bAirActive)
		{
			m_bAirActive = false;
			m_vAirVelocityXZ = {};
			m_vVelocity = { 0.f, 0.f, 0.f };
		}
	}
	//공중. 잠깐 뜬 건 무시하고 DELAY_TIME 이상 떠 있어야 공중 상태
	else
	{
		m_fAirTime += fTimeDelta;
		if (m_bAirActive == false && m_fAirTime >= DELAY_TIME)
		{
			m_bAirActive = true;
			m_vAirVelocityXZ = m_vLastGroundedXZ;
		}
	}
}

//자동 이동. "곡선 위 목표 위치 - 현재 위치"를 이동량으로 넘겨서 벽에 막혀도 오차가 안 쌓임
void CMovement::Compute_MoveTo(_float fTimeDelta)
{
	m_fMoveElapsedTime += fTimeDelta;

	_float fTime = min(m_fMoveElapsedTime / m_fMoveDuration, 1.f);
	_float fEaseValue = Ease(m_eMoveEaseType, fTime);

	_vector vNextPos = XMVectorLerp(XMLoadFloat4(&m_vMoveStartPos), XMLoadFloat4(&m_vMoveTargetPos), fEaseValue);
	_vector vDeltaPos = vNextPos - m_pOwnerTransformCom->Get_State(STATE::POSITION);

	_float fMoveY = m_bMoveTo3D ? XMVectorGetY(vDeltaPos) : 0.f;
	m_tMovementResult.vMoveDelta = _float3(XMVectorGetX(vDeltaPos), fMoveY, XMVectorGetZ(vDeltaPos));
	m_vVelocity = { 0.f, 0.f, 0.f };

	_vector vTargetQuat = XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&m_vMoveStartQuat), XMLoadFloat4(&m_vMoveTargetQuat), fEaseValue));
	m_tMovementResult.bChangedQuat = true;
	XMStoreFloat4(&m_tMovementResult.vMoveQuat, vTargetQuat);

	//끝까지 갔거나 옆이 막히면 종료
	if (fTime >= 1.f || m_bSideBlocked)
	{
		m_bMoveTo = false;
		m_bMoveReached = true;
	}
}

void CMovement::Compute_RootMotion(_float fTimeDelta)
{
	_float fTurnAlpha = 1.f - expf(-m_fTurnRate * fTimeDelta);
	_vector vNextQuat = {};

	//락온이면 타켓 방향으로 회전
	_vector vToTarget = XMVectorZero();
	if (m_pOwnerStatusCom->Is_CanTargetTracking() && Get_LockOnDir(vToTarget))
	{
		vNextQuat = Get_Yaw_Quat(vToTarget);
		XMStoreFloat4(&m_vRootMotionQuat, vNextQuat);

		m_tMovementResult.bChangedQuat = true;
		XMStoreFloat4(&m_tMovementResult.vMoveQuat, vNextQuat);
	}
	else
	{
		vNextQuat = XMQuaternionSlerp(m_pOwnerTransformCom->Get_Quat(), XMLoadFloat4(&m_vRootMotionQuat), fTurnAlpha);
	}

	//애니메이션 이동량에 배율을 곱하고, 고정해 둔 방향 기준으로 월드로 돌림
	_vector vLocalDelta = XMLoadFloat4(&m_tRootMotionTransform.vTranslation) * m_fRootMotionScale;
	_vector vWorldDelta = XMVector3Rotate(vLocalDelta, vNextQuat);

	m_tMovementResult.vMoveDelta = _float3(XMVectorGetX(vWorldDelta), XMVectorGetY(vWorldDelta), XMVectorGetZ(vWorldDelta));
	m_vVelocity = { 0.f, 0.f, 0.f };

	//공중 루트모션의 경우 수평속도를 관성으로 계속 갱신함
	if (m_bAirActive && fTimeDelta > 0.f)
		m_vAirVelocityXZ = _float2(m_tMovementResult.vMoveDelta.x / fTimeDelta, m_tMovementResult.vMoveDelta.z / fTimeDelta);
}

void CMovement::Compute_AirControl(_float fTimeDelta)
{
	//입력이 있으면 AIR_CONTROL_SPEED 쪽으로 서서히만 틀 수 있음
	if (m_bHasMoveInput)
	{
		_vector vTarget = XMLoadFloat4(&m_vCamInputDir) * AIR_CONTROL_SPEED;
		_vector vCur = XMVectorSet(m_vAirVelocityXZ.x, 0.f, m_vAirVelocityXZ.y, 0.f);

		_float fAlpha = 1.f - expf(-AIR_CONTROL_RATE * fTimeDelta);
		vCur = XMVectorLerp(vCur, vTarget, fAlpha);
		m_vAirVelocityXZ = _float2(XMVectorGetX(vCur), XMVectorGetZ(vCur));
	}

	_float fAirTimeDelta = fTimeDelta * m_fAirTimeScale;
	m_tMovementResult.vMoveDelta = _float3(m_vAirVelocityXZ.x * fAirTimeDelta, 0.f, m_vAirVelocityXZ.y * fAirTimeDelta);
	m_vVelocity = { 0.f, 0.f, 0.f };
}

void CMovement::Compute_GroundMove(_float fTimeDelta)
{
	//이동 잠금 중엔 속도도 0
	if (m_pOwnerStatusCom->CanMove() == false)
	{
		m_vVelocity = { 0.f, 0.f, 0.f };
		return;
	}

	_vector vMoveDir = XMLoadFloat4(&m_vCamInputDir);
	_vector vTargetVelocity = Get_CurrentSpeed() * vMoveDir;
	_vector vVelocity = XMLoadFloat3(&m_vVelocity);

	//입력이 있으면 가속, 없으면 감속
	if (m_bHasMoveInput)
	{
		//이전 velocity와 비교하여 반대 방향으로 전환된다면 빠른 전환을 위해 가속을 키움
		_float fReverseDot = XMVectorGetX(XMVector3Dot(vMoveDir, vVelocity));
		_float fAccel = (fReverseDot < 0.f) ? m_fAcceleration * REVERSE_MULT_ACCEL : m_fAcceleration;

		_float fAccelAlpha = 1.f - expf(-fAccel * fTimeDelta);
		vVelocity = XMVectorLerp(vVelocity, vTargetVelocity, fAccelAlpha);
	}
	else
	{
		_float fDecelAlpha = 1.f - expf(-m_fDeceleration * fTimeDelta);
		vVelocity = XMVectorLerp(vVelocity, XMVectorZero(), fDecelAlpha);
		if (XMVectorGetX(XMVector3Length(vVelocity)) < 0.01f) vVelocity = XMVectorZero();
	}

	XMStoreFloat3(&m_vVelocity, vVelocity);
	_vector vDeltaVelocity = vVelocity * fTimeDelta;
	m_tMovementResult.vMoveDelta = _float3(XMVectorGetX(vDeltaVelocity), XMVectorGetY(vDeltaVelocity), XMVectorGetZ(vDeltaVelocity));
}

//지상 전용 회전: 락온이면 타겟 쪽, 아니면 이동 입력 방향
void CMovement::Compute_Facing(_float fTimeDelta)
{
	_vector vFaceDir = XMVectorZero();
	_bool bFace = false;

	// 락온이면 타겟 추적. 질주는 추적 제외
	if (m_pOwnerStatusCom->Is_LockedOn() && m_pOwnerStatusCom->Is_CanTargetTracking() && Is_Sprinting() == false)
	{
		bFace = Get_LockOnDir(vFaceDir);
	}
	// 이동 입력으로 회전
	else if (m_pOwnerStatusCom->CanRotate() && m_bHasMoveInput)
	{
		vFaceDir = XMLoadFloat4(&m_vCamInputDir);
		bFace = true;
	}

	// 그 외는 회전 X
	if (bFace == false) return;

	_float fTurnAlpha = 1.f - expf(-m_fTurnRate * fTimeDelta);
	_vector vResultQuat = XMQuaternionNormalize(XMQuaternionSlerp(m_pOwnerTransformCom->Get_Quat(), Get_Yaw_Quat(vFaceDir), fTurnAlpha));

	m_tMovementResult.bChangedQuat = true;
	XMStoreFloat4(&m_tMovementResult.vMoveQuat, vResultQuat);
}

void CMovement::Apply_Gravity(_float fTimeDelta, _bool bGrounded, _bool bGravityDisabled)
{
	if (bGravityDisabled)
	{
		if (fTimeDelta > 0.f)
			m_fFallVelocityY = m_tMovementResult.vMoveDelta.y / fTimeDelta;
		return;
	}

	_float fAirTimeDelta = fTimeDelta * m_fAirTimeScale;

	//땅: 아래로 살짝 눌러 바닥에 붙임 / 공중: 무게만큼 중력 가속, 최대 낙하 속도 제한
	if (bGrounded)
	{
		m_fFallVelocityY = FALL_VALUE;
	}
	else
	{
		m_fFallVelocityY += Get_FallGravity() * fAirTimeDelta;
		if (m_fFallVelocityY < MAX_FALL_SPEED) m_fFallVelocityY = MAX_FALL_SPEED;
	}

	m_tMovementResult.vMoveDelta.y += m_fFallVelocityY * fAirTimeDelta;
}

_bool CMovement::Get_LockOnDir(_vector& OutDir) const
{
	if (m_pOwnerStatusCom->Is_LockedOn() == false) return false;

	CAIObject* pLockTarget = m_pOwnerStatusCom->Get_LockedTarget();
	if (pLockTarget == nullptr) return false;

	_vector vPlayerPos = m_pOwnerTransformCom->Get_State(STATE::POSITION);
	_vector vTargetPos = pLockTarget->Get_Transform()->Get_State(STATE::POSITION);
	_vector vToTarget = XMVectorSetY(vTargetPos - vPlayerPos, 0.f);

	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vToTarget)), 0.f)) return false;

	OutDir = XMVector3Normalize(vToTarget);
	return true;
}

_vector CMovement::Get_Yaw_Quat(_fvector vDir)
{
	return XMQuaternionRotationRollPitchYaw(0.f, atan2f(XMVectorGetX(vDir), XMVectorGetZ(vDir)), 0.f);
}


void CMovement::Begin_MoveTo(_fvector vTargetPos, _fvector vTargetQuat, _float fDuration, EASE_TYPE eEaseType, _bool b3DMove)
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
	m_bMoveTo3D = b3DMove;
}

void CMovement::Begin_KnockBack(_fvector vDirection, _float fDistance, _float fDuration, EASE_TYPE eEaseType)
{
	_vector vDir = XMVector3Normalize(XMVectorSetY(vDirection, 0.f));
	_vector vTargetPos = m_pOwnerTransformCom->Get_State(STATE::POSITION) + vDir * fDistance;
	//넉백이면 현재 회전 유지 
	Begin_MoveTo(vTargetPos, m_pOwnerTransformCom->Get_Quat(), fDuration, eEaseType);
}

void CMovement::Stop_MoveTo()
{
	m_bMoveTo = false;
}

void CMovement::Clear()
{
	m_fFallVelocityY = FALL_VALUE;
	m_vVelocity = { 0.f, 0.f, 0.f };
	m_bAirActive = false;
	m_fAirTime = 0.f;
	m_vAirVelocityXZ = { 0.f, 0.f };
	m_vLastGroundedXZ = { 0.f, 0.f };
	m_fAirTimeScale = 1.f;

	//자동 이동·루트모션도 끊음
	Stop_MoveTo();
	m_bSideBlocked = false;
	m_bPlayRootMotion = false;
	m_bRootMotionStarted = false;
	m_bRootMotionRestart = false;
	m_fRootMotionScale = 1.f;
}

void CMovement::Launch(_fvector vVelocity)
{
	m_vAirVelocityXZ = _float2(XMVectorGetX(vVelocity), XMVectorGetZ(vVelocity));
	m_fFallVelocityY = XMVectorGetY(vVelocity);
	m_fAirTime = DELAY_TIME;
	m_bAirActive = true;
	m_vVelocity = { 0.f, 0.f, 0.f };
}


CMovement* CMovement::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMovement* pInstance = new CMovement(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Movement Create Failed");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CMovement::Clone(void* pArg)
{
	CMovement* pInstance = new CMovement(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Movement Clone Failed");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMovement::Free()
{
	__super::Free();
}

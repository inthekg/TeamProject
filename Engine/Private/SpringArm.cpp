#include "SpringArm.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine_Utility.h"
#include "GameInstance.h"

CSpringArm::CSpringArm()
	:CComponent(nullptr, nullptr)
{

}

CSpringArm::CSpringArm(const CSpringArm& Prototype)
	:CComponent(Prototype)
{
}

HRESULT CSpringArm::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CSpringArm::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	SPRING_DESC* pDesc = static_cast<SPRING_DESC*>(pArg);
	m_fTargetOffset = pDesc->fTargetOffset;
	m_fArmLength = pDesc->fArmLength;
	m_vArmRotLimits = pDesc->vArmRotLimits;
	m_fMouseSensor = pDesc->fMouseSensor;

	if (pDesc->pOwner == nullptr) return E_FAIL;
	m_pOwnerTransform = pDesc->pOwner->Get_Transform();

	Clear();

	return S_OK;
}

void CSpringArm::Update(_long lVertical_MoveDelta, _long lHorizon_MoveDelta, _float fTimeDelta)
{
	if (m_bLockOn) return;
	if (lVertical_MoveDelta)
	{
		//마우스 이동량은 이미 '이번 프레임 동안 움직인 양'이라 fTimeDelta 곱하지 않음
		m_fVerticalValue += lVertical_MoveDelta * m_fMouseSensor;
		m_fVerticalValue = fClamp(m_fVerticalValue, m_vArmRotLimits.x, m_vArmRotLimits.y);
	}

	if (lHorizon_MoveDelta)
	{
		m_fHorizonValue += lHorizon_MoveDelta * m_fMouseSensor;
		m_fHorizonValue = XMScalarModAngle(m_fHorizonValue); // -PI~PI 로 Wrapping
	}
}

//Owner Physical Update 이후에 호출하기 
void CSpringArm::Late_Update(_float fTimeDelta)
{
	Update_TargetPos(fTimeDelta);
	Update_Arm(fTimeDelta);
} 

void CSpringArm::Update_TargetPos(_float fTimeDelta)
{
	_float4 vPos = {};
	XMStoreFloat4(&vPos, m_pOwnerTransform->Get_State(STATE::POSITION));
	vPos.y += m_fTargetOffset + m_fPivotExtraY;

	_float fSmoothTime = m_bLockOn ? LOCKON_SMOOTH_TIME : SMOOTH_TIME;
	_vector vLerpPos = SmoothDamp(m_vCurrentPos, vPos, m_vCurrentVelocity, fSmoothTime, fTimeDelta);

	_vector vTargetPos = XMLoadFloat4(&vPos);
	//최대 카메라 Lag 거리 제한 
	_vector vLagOff = XMVector3ClampLength(vLerpPos - vTargetPos, 0.f, CAM_LAG_MAX_DIST);
	vLerpPos = vTargetPos + vLagOff;
	XMStoreFloat4(&m_vCurrentPos, vLerpPos);
}

void CSpringArm::Set_LockOn(_bool bLockOn, _float3 vTargetPos)
{
	m_bLockOn = bLockOn;
	if (bLockOn) m_vLockOnTargetPos = vTargetPos;
}

void CSpringArm::Clear()
{
	_float4 vOwnerLook;
	XMStoreFloat4(&vOwnerLook, m_pOwnerTransform->Get_State(STATE::LOOK));
	m_fHorizonValue = atan2f(vOwnerLook.x, vOwnerLook.z);
	m_fVerticalValue = XMConvertToRadians(10.f);

	m_fCollisionDist = -1.f;
	m_bLockOn = false;

	// 클리어 시에는 이전 위치에서 보간하지않도록 현재 위치로 설정 
	_float4 vTargetPos = {};
	XMStoreFloat4(&vTargetPos, m_pOwnerTransform->Get_State(STATE::POSITION));
	vTargetPos.y += m_fTargetOffset;
	m_vCurrentPos = vTargetPos;
	m_vCurrentVelocity = {};

	m_bAdjusting = false;
	m_fAdjustAlpha = 0.f;
	m_AdjustDesc = {};
	m_fPivotExtraY = 0.f;

	Update_Arm(0.f);
}

void CSpringArm::Sync_FromQuat(_fvector vCamQuat)
{
	_float4 vLook = {};
	XMStoreFloat4(&vLook, XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), vCamQuat));

	//카메라의 시야축인 Z축이 기준 
	m_fHorizonValue = atan2f(vLook.x, vLook.z);

	const _float fLengthXZ = sqrtf(vLook.x * vLook.x + vLook.z * vLook.z);
	m_fVerticalValue = fClamp(atan2f(-vLook.y, fLengthXZ), m_vArmRotLimits.x, m_vArmRotLimits.y);

	m_bLockOn = false;

	_float4 vTargetPos = {};
	XMStoreFloat4(&vTargetPos, m_pOwnerTransform->Get_State(STATE::POSITION));
	vTargetPos.y += m_fTargetOffset + m_fPivotExtraY;
	m_vCurrentPos = vTargetPos;
	m_vCurrentVelocity = {};

	Update_Arm(0.f);
}

void CSpringArm::Begin_Adjust(const SPRING_ADJUST_DESC& AdjustDesc)
{
	m_AdjustDesc = AdjustDesc;

	if (m_AdjustDesc.fBlendInTime < 0.01f)	m_AdjustDesc.fBlendInTime = 0.01f;
	if (m_AdjustDesc.fBlendOutTime < 0.01f)	m_AdjustDesc.fBlendOutTime = 0.01f;
	m_bAdjusting = true;
}

void CSpringArm::End_Adjust()
{
	m_bAdjusting = false;
}

_vector CSpringArm::SmoothDamp(_float4 vCurrent, _float4 vTarget, _float4& vCurrentVelocity, _float fSmoothTime, _float fDeltaTime)
{
	_float fOmega	= 2.f / fSmoothTime;
	_float fX = fOmega * fDeltaTime;
	_float fExp = 1.f / (1.f + fX + 0.48f * fX * fX + 0.235f * fX * fX * fX);
	_vector vChange = XMLoadFloat4(&vCurrent) - XMLoadFloat4(&vTarget);
	_vector vCurVelocity = XMLoadFloat4(&vCurrentVelocity);

	_vector vTemp = (vCurVelocity + (fOmega * vChange)) * fDeltaTime;

	vCurVelocity = (vCurVelocity - (fOmega * vTemp)) * fExp;
	XMStoreFloat4(&vCurrentVelocity, vCurVelocity);

	_vector vResult = XMLoadFloat4(&vTarget) + (vChange + vTemp) * fExp;
	return vResult;
}

void CSpringArm::Update_Arm(_float fTimeDelta)
{
	if (m_bLockOn)
	{
		_vector vPivot = XMLoadFloat4(&m_vCurrentPos);
		_vector vTarget = XMLoadFloat3(&m_vLockOnTargetPos);
			
		_vector vToTarget = vTarget - vPivot;
		_float fHorizonDist = XMVectorGetX(XMVector3Length(XMVectorSetY(vToTarget, 0.f)));
		
		_float fTargetPitch = LOCKON_PITCH - atan2f(XMVectorGetY(vToTarget),fHorizonDist);
		fTargetPitch = fClamp(fTargetPitch, LOCKON_PITCH_MIN, LOCKON_PITCH_MAX);

		//락온일 때는 마우스 방향이 아니라 대상을 바라보도록 캐릭터 뒤에 위치
		_float fTargetYaw = m_fHorizonValue;
		if (fHorizonDist > 0.001f)
			fTargetYaw = atan2f(XMVectorGetX(vToTarget), XMVectorGetZ(vToTarget));

		m_fVerticalValue = Approach_Angle(m_fVerticalValue, fTargetPitch, LOCKON_DEADZONE_PITCH, LOCKON_PITCH_SPEED_MAX, LOCKON_BLEND_RATE, fTimeDelta);
		m_fHorizonValue = Approach_Angle(m_fHorizonValue, fTargetYaw, LOCKON_DEADZONE_YAW, LOCKON_YAW_SPEED_MAX, LOCKON_BLEND_RATE, fTimeDelta);

		//m_fHorizonValue 는 각도 제한이 없으므로 다시 -PI~PI로 Wrapping
		m_fHorizonValue = XMScalarModAngle(m_fHorizonValue);
	}

	_float fFinalPitch = m_fVerticalValue;
	_float fFinalYaw = m_fHorizonValue;
	_float fFinalArmLength = m_fArmLength;

	//액션 킬 떄 0->1로 , 끌 때 1->0로 보간
	_float fAlphaTarget = m_bAdjusting ? 1.f : 0.f;
	if (m_fAdjustAlpha != fAlphaTarget)
	{
		_float fBlendTime = (fAlphaTarget > m_fAdjustAlpha)
			? m_AdjustDesc.fBlendInTime : m_AdjustDesc.fBlendOutTime;

		_float fStep = fTimeDelta / fBlendTime;
		//목표위치 - 현재  =  목표량
		m_fAdjustAlpha += fClamp(fAlphaTarget - m_fAdjustAlpha, -fStep, fStep);
	}

	_float fAdjustEase = Ease(EASE_TYPE::OUT_CUBIC, m_fAdjustAlpha);

	if (m_fAdjustAlpha > 0.f)
	{
		//타켓 룩엣에 맞게 회전 보정 x 배율 + 클램프
		if (m_AdjustDesc.bLookBias)
		{
			_vector vToTarget = XMLoadFloat3(&m_AdjustDesc.vLookTargetPos) - XMLoadFloat4(&m_vCurrentPos);
			_float fLenXZ = XMVectorGetX(XMVector3Length(XMVectorSetY(vToTarget, 0.f)));
			_float fTargetPitch = atan2f(-XMVectorGetY(vToTarget), fLenXZ);
			//Yaw니까 Z기준 각도
			_float fTargetYaw = atan2f(XMVectorGetX(vToTarget), XMVectorGetZ(vToTarget));

			//XMScalarModAngle : 각도 차이를 구할 때 제일 짧은 회전 경로를 반환함
			_float fDPitch = fClamp(XMScalarModAngle(fTargetPitch - fFinalPitch) * m_AdjustDesc.fPitchMult, -m_AdjustDesc.fMaxAngle, m_AdjustDesc.fMaxAngle);
			_float fDYaw = fClamp(XMScalarModAngle(fTargetYaw - fFinalYaw) * m_AdjustDesc.fYawMult, -m_AdjustDesc.fMaxAngle, m_AdjustDesc.fMaxAngle);

			fFinalPitch += fDPitch * fAdjustEase;
			fFinalYaw += fDYaw * fAdjustEase;
		}

		if (m_AdjustDesc.fArmLength > 0.f)
		{
			fFinalArmLength += (m_AdjustDesc.fArmLength - m_fArmLength) * fAdjustEase;
		}
	}

	_vector vTargetQuat = XMQuaternionRotationRollPitchYaw(fFinalPitch, fFinalYaw, 0.f);
	XMMATRIX matRot = XMMatrixRotationQuaternion(vTargetQuat);
	_vector vArmDir = XMVector3TransformNormal(XMVectorSet(0.f, 0.f, -1.f, 0.f), matRot);

	XMVECTOR vFinalPos = XMLoadFloat4(&m_vCurrentPos) + (vArmDir * fFinalArmLength);

	//로컬 위치 오프셋
	if (m_fAdjustAlpha > 0.f)
	{
		_vector vOffset = XMVector3TransformNormal(XMLoadFloat3(&m_AdjustDesc.vLocalOffset), matRot);
		vFinalPos += vOffset * fAdjustEase;
	}

	Update_Collision(fTimeDelta, XMLoadFloat4(&m_vCurrentPos), vFinalPos);

	XMStoreFloat4(&m_FinalCameraPose.vPosition, vFinalPos);
	XMStoreFloat4(&m_FinalCameraPose.vQuat, vTargetQuat);
}

void CSpringArm::Update_Collision(_float fTimeDelta, _fvector vCurPos, _vector& vFinalPos)
{
	_vector vToCam = vFinalPos - vCurPos;
	_float fDist = XMVectorGetX(XMVector3Length(vToCam));
	if (IsEqual(fDist, 0.f)) return;

	_vector vDir = XMVector3Normalize(vToCam);
	CollisionResult Result;
	_float fAllowedDist = fDist;

	FILTER_GROUP eFilter = static_cast<FILTER_GROUP>( ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));
	if (m_pGameInstance->Sweep_Sphere(CAM_RADIUS, vCurPos, vDir, fDist, eFilter, Result, PxQueryFlag::eSTATIC))
	{
		fAllowedDist = fClamp(Result.tSweepData.fDistance, COL_DIST_MIN, fDist);
	}

	//초기화 된적이 없다면 
	if (m_fCollisionDist < 0.f)			
		m_fCollisionDist = fAllowedDist;

	if(fAllowedDist < m_fCollisionDist)
	{
		m_fCollisionDist = fAllowedDist;
	}
	else
	{
		m_fCollisionDist += COL_RECOVER_RATE * fTimeDelta;
		m_fCollisionDist = fClamp(m_fCollisionDist, 0.f, fAllowedDist);
	}

	vFinalPos = vCurPos + vDir * m_fCollisionDist;
}

_float CSpringArm::Approach_Angle(_float fCurrent, _float fTarget, _float fDeadZone, _float fMaxSpeed, _float fRate, _float fTimeDelta)
{
	//회전 오차값 
	_float fDiff = XMScalarModAngle(fTarget - fCurrent);

	//오차가 DeadZone보다 작으면 회전하지 않음
	if (fabsf(fDiff) < fDeadZone)
		return fCurrent;

	fDiff -= (fDiff < 0.f) ? -fDeadZone : fDeadZone;
	// 프레임 독립
	_float fStep = fDiff * (1.f - expf(-fRate * fTimeDelta));


	//최대 회전속도 제한
	const _float fMaxStep = fMaxSpeed * fTimeDelta;
	fStep = fClamp(fStep, -fMaxStep, fMaxStep);

	return fCurrent + fStep;
}

CSpringArm* CSpringArm::Create()
{
	CSpringArm* pInstance = new CSpringArm();
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CSpringArm");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CSpringArm::Clone(void* pArg)
{
	CSpringArm* pInstance = new CSpringArm(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CSpringArm");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CSpringArm::Free()
{
	__super::Free();
}

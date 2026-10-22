#include "Monster_BigDog.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"
#include "Model.h"
#include "Mesh.h"
#include "Animator.h"
#include "Navigation.h"

#include "StatComponent_AI.h"
#include "Movement_AI.h"
#include "Monster_Include.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_BigDog, CMonster_Melee)
END_REFLECTION(CMonster_BigDog)
#endif

static const PATTERN_DEF s_Patterns[] =
{
	{ 1, ATK_GROUP::SHORT, 1.f, 6.f },
	{ 2, ATK_GROUP::SHORT, 1.f, 6.f },
	{ 3, ATK_GROUP::SHORT, 1.f, 6.f },
	{ 4, ATK_GROUP::SHORT, 1.f, 6.f },
	{ 5, ATK_GROUP::LONG,  0.f, 8.f },
	{ 6, ATK_GROUP::LONG,  1.f, 7.f },
	{ 7, ATK_GROUP::LONG,  1.f, 7.f },
	{ 8, ATK_GROUP::SHORT, 1.f, 6.f },
	{ 9, ATK_GROUP::SHORT, 1.f, 6.f },
};

CMonster_BigDog::CMonster_BigDog(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Melee(pDevice, pContext)
{
}

CMonster_BigDog::CMonster_BigDog(const CMonster_BigDog& Prototype)
	: CMonster_Melee(Prototype)
{
}

HRESULT CMonster_BigDog::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_BigDog.json";
	m_strFootStepSurface = "Dirt";
	m_pPatterns = s_Patterns;
	m_iPatternDefCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_BigDog::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_BigDog");
		return E_FAIL;
	}

	MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);

	Apply_MeleeDesc(*pDesc);
	m_fFarAttackRange = pDesc->fFarAttackRange;
	m_fRunChaseRange = pDesc->fRunChaseRange;
	m_fApproachRange = pDesc->fApproachRange;
	m_fSkillMoveSpeed = pDesc->fSkillMoveSpeed;
	m_eRearMode = pDesc->eRearMode;
	m_fRearTailRange = pDesc->fRearTailRange;

	Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	CHARACTER_SETUP_DESC CharDesc{};
	CharDesc.strModelPrototypeTag = pDesc->strModelProtoTag;
	CharDesc.strAnimFolderTag = pDesc->strAnimFolderTag;
	CharDesc.strAnimTag = pDesc->strAnimFolderTag;
	CharDesc.pRootBoneName = pDesc->pRootBoneName;
	CharDesc.bUseCCT = true;
	CharDesc.fRunSpeed = pDesc->fWalkSpeed;
	CharDesc.fSprintSpeed = pDesc->fChaseSpeed;
	CharDesc.eControllerType = CCT_TYPE::BOX;
	CharDesc.vCCTBoxHalf = { 1.8f, 0.9f, 1.8f };

	if (nullptr != pArg)
		CharDesc.iModelLevel = static_cast<AIOBJECT_DESC*>(pArg)->iProtoLevel;
	else
		CharDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_HitBoxes()))
		return E_FAIL;

	m_bSpawned = true;
	m_fPerceiveFOVDegree = 270.f;
	m_fPerceiveEyeHeight = 1.5f;

	Spawn();

	return S_OK;
}

void CMonster_BigDog::Spawn()
{
	__super::Spawn();

	m_bTurning = false;
	m_bSpawned = true;
	m_bRearSnapshot = false;
	m_bWasIdle = false;
	m_bAggroLossActive = false;
	m_bMoving = false;
	m_bMustAttack = false;
	m_bCounting = false;
	m_fCheckTimer = 0.f;
	m_iTurnDir = static_cast<_int>(MON_DIRECTION::FRONT);
	Set_EmissiveAmount(1.f);
}

void CMonster_BigDog::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
	{
		_vector vBefore = (nullptr != m_pTransformCom) ? m_pTransformCom->Get_State(STATE::POSITION) : XMVectorZero();
		_vector vQuatBefore = (nullptr != m_pTransformCom) ? m_pTransformCom->Get_Quat() : XMQuaternionIdentity();

		Apply_Movment(fTimeDelta);

		if (nullptr != m_pTransformCom)
		{
			_vector vAfter = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vQuatAfter = m_pTransformCom->Get_Quat();

			_float fMoveLen = XMVectorGetX(XMVector3Length(XMVectorSubtract(vAfter, vBefore)));

			_vector vQuatDelta = XMQuaternionMultiply(XMQuaternionInverse(vQuatBefore), vQuatAfter);
			_vector vAxis; _float fAngle;
			XMQuaternionToAxisAngle(&vAxis, &fAngle, XMQuaternionNormalize(vQuatDelta));
			if (fAngle > XM_PI) fAngle -= XM_2PI;
			_float fYawDegPerSec = (fTimeDelta > 1e-5f) ? XMConvertToDegrees(fAngle) / fTimeDelta : 0.f;

			static _float s_fMoveLogT = 0.f;
			s_fMoveLogT += fTimeDelta;
			const _bool bSpike = (fMoveLen > 0.3f) || (fabsf(fYawDegPerSec) > 720.f);
			if (true == bSpike || s_fMoveLogT >= 0.5f)
			{
				s_fMoveLogT = 0.f;
				const ANIM_CLIP_SETTING* pClip = (nullptr != m_pAnimator) ? m_pAnimator->Get_CurClip() : nullptr;
				wchar_t szBuf[256];
				swprintf_s(szBuf, L"[BigDog][MOVE] moveLen=%.3f yawDegPerSec=%.1f root=%d hitActive=%d clip=%hs spike=%d\n",
					fMoveLen, fYawDegPerSec,
					(nullptr != pClip && pClip->bUseRootMotion) ? 1 : 0,
					(_int)Is_HitActive(),
					(nullptr != pClip) ? pClip->strExportName.c_str() : "none",
					(_int)bSpike);
				OutputDebugStringW(szBuf);
			}
		}
	}
}

void CMonster_BigDog::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_BigDog::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
	{
		m_HitBoxes.On_StateChanged();
		m_HitBoxes.Set_HitType(HIT_TYPE::Normal);
	}

	m_HitBoxes.Update(m_pGameInstance, this, fTimeDelta);
}

HRESULT CMonster_BigDog::Render()
{
	return __super::Render();
}

void CMonster_BigDog::On_CheckDead()
{
	m_pStatCom->Set_FatalDying(false);

	if (Is_Dead())
		m_bDead = true;

	Set_Groggy(false);
	m_pBlackBoard->Set<_bool>(L"bFatal", false);

	m_pStatCom->Reset_Groggy();
	m_pStatCom->Set_Staggerable(false);

	Request_AnimAction(L"FATAL_AFTER");
}

void CMonster_BigDog::Set_FatalMode(_bool bMode)
{
	const _bool bWasFatal = (nullptr != m_pBlackBoard) && m_pBlackBoard->Get<_bool>(L"bFatal");

	if (true == bMode && false == bWasFatal)
	{
		Request_AnimAction(L"FATAL");
		Set_AnimFloat(L"Speed", 0.f);

		m_bMoving = false;
		m_bMustAttack = false;
		m_bAggro = true;

		Set_Groggy(false);
	}

	if (false == bMode)
	{
		if (true == Is_Dead())
		{
			m_pStatCom->Set_FatalDying(true);
		}
	}

	m_pBlackBoard->Set<_bool>(L"bFatal", bMode);
}

void CMonster_BigDog::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	Update_CombatParams(fTimeDelta);
	Update_Activation();
	Update_AttackLoop(fTimeDelta);

	if (true == m_pBlackBoard->Get<_bool>(L"bFatal"))
	{
		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();
		return;
	}

	if (true == m_bDead || m_pStatCom->Is_Dead())
	{
		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();

		const _bool bInDeadPose =
			Is_AnimInState(L"DEAD_F") || Is_AnimInState(L"DEAD_HIT_F") || Is_AnimInState(L"DEAD_HIT_B");
		if (false == bInDeadPose)
			Request_AnimAction(L"DEAD");

		return;
	}

	if (true == m_pBlackBoard->Get<_bool>(L"bGroggy"))
	{
		Set_AnimFloat(L"Speed", 0.f);
		m_pBlackBoard->Set<_bool>(L"bHit", false);
		m_fHitTimer = 0.f;
		m_bMoving = false;
		m_bMustAttack = false;
		Mirror_ControllerParams();
		return;
	}

	if (m_pBlackBoard->Get<_bool>(L"bHit"))
	{
		m_fHitTimer += fTimeDelta;
		const _bool bFreshHit = Consume_HitRefresh();
		if (true == bFreshHit)
			m_fHitTimer = 0.f;
		const _bool bInReact =
			Is_AnimInState(L"DAMAGE_BACK") || Is_AnimInState(L"DAMAGE_LEFT") || Is_AnimInState(L"DAMAGE_RIGHT") ||
			Is_AnimInState(L"KNOCKBACK_BACK") || Is_AnimInState(L"KNOCKBACK_LEFT") || Is_AnimInState(L"KNOCKBACK_RIGHT");
		if (false == bInReact || true == bFreshHit)
			Fire_Knockback();
		if (m_fHitTimer >= m_fHitMaxTime) { m_pBlackBoard->Set<_bool>(L"bHit", false); m_fHitTimer = 0.f; }
		m_bMoving = false;
		m_bMustAttack = false;
		Mirror_ControllerParams();
		return;
	}
	m_fHitTimer = 0.f;

	_vector vSpawn = Get_SpawnPos();

	if (false == m_bAggro)
	{
		if (false == m_bAggroLossActive)
		{
			_float fDistToSpawn = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vSpawn, m_pTransformCom->Get_State(STATE::POSITION)), 0.f)));

			if (fDistToSpawn > 0.5f)
			{
				if (true == Can_AnimMove())
				{
					Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
					Set_AnimFloat(L"Speed", 1.f);
					m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
					m_pMovementCom->MoveTo_Navi(vSpawn, fTimeDelta);
				}
				else
				{
					Set_AnimFloat(L"Speed", 0.f);
				}

				Mirror_ControllerParams();
				return;
			}

			m_bAggroLossActive = true;

			wchar_t szBuf[64];
			swprintf_s(szBuf, L"[BigDog][EVT] AGGRO_LOST_HOME\n");
			OutputDebugStringW(szBuf);

			if (m_pMovementCom) m_pMovementCom->Stop_MoveTo();
			m_bMoving = false; m_bMustAttack = false; m_bCancelConsumed = false;
			m_iPendingPatternID = -1; m_bApproaching = false;
			m_fMoveTimer = 0.f; m_fHitTimer = 0.f; m_fIdleTimer = 0.f;
		}

		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();
		return;
	}

	m_bAggroLossActive = false;

	m_bReturningPrev = Is_Returning();
	if (true == Update_Leash(fTimeDelta, m_fDistToTarget))
	{
		if (false == m_bReturningPrev)
		{
			if (m_pMovementCom) m_pMovementCom->Stop_MoveTo();
			m_bMoving = false; m_bMustAttack = false; m_bCancelConsumed = false;
			m_iPendingPatternID = -1; m_bApproaching = false;
			m_fMoveTimer = 0.f; m_fHitTimer = 0.f; m_fIdleTimer = 0.f;
		}

		if (true == Can_AnimMove())
		{
			Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
			Set_AnimFloat(L"Speed", 1.f);
			m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
			m_pMovementCom->MoveTo_Navi(vSpawn, fTimeDelta);
		}
		else
		{
			Set_AnimFloat(L"Speed", 0.f);
		}

		Mirror_ControllerParams();
		return;
	}

	Update_PatternSelect(fTimeDelta);
	Drive_Steering_BigDog(fTimeDelta);
	Mirror_ControllerParams();
}

void CMonster_BigDog::Action_Notify(_uint64 iHashKey)
{
	__super::Action_Notify(iHashKey);

	auto itType = m_HitTypeKeys.find(iHashKey);
	if (itType != m_HitTypeKeys.end())
		m_HitBoxes.Set_HitType(itType->second);

	m_HitBoxes.Handle_Notify(iHashKey);

	if (iHashKey == Hash_String(L"CHECK_COUNT"))
	{
		m_bCounting = true;
		m_fCheckTimer = 0.f;
	}
}

void CMonster_BigDog::Update_AttackLoop(_float fTimeDelta)
{
	if (false == m_bCounting)
		return;

	m_fCheckTimer += fTimeDelta;

	Set_AnimFloat(L"Timer", m_fCheckTimer);
	Set_AnimBool(L"InRange", m_fDistToTarget <= m_fApproachRange);

	if (m_fDistToTarget <= m_fApproachRange || m_fCheckTimer >= 1.5f)
		m_bCounting = false;
}

_bool CMonster_BigDog::Is_RearPattern(_int iPatternID) const
{
	return (iPatternID >= 6 && iPatternID <= 9);
}

_bool CMonster_BigDog::Is_TargetInFront()
{
	if (nullptr == m_pTransformCom)
		return true;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return true;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
		return true;

	vTo = XMVector3Normalize(vTo);
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	return XMVectorGetX(XMVector3Dot(vLook, vTo)) >= 0.f;
}

_bool CMonster_BigDog::Is_TargetInRear()
{
	if (nullptr == m_pTransformCom)
		return false;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return false;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
		return false;

	vTo = XMVector3Normalize(vTo);
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	const _float fDot = XMVectorGetX(XMVector3Dot(vLook, vTo));
	return fDot <= -cosf(XMConvertToRadians(m_fRearConeDeg));
}

_bool CMonster_BigDog::Is_BackAligned()
{
	if (nullptr == m_pTransformCom)
		return true;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return true;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
		return true;

	vTo = XMVector3Normalize(vTo);
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	const _float fDot = XMVectorGetX(XMVector3Dot(vLook, vTo));
	return fDot <= -cosf(XMConvertToRadians(m_fAttackStartDeg));
}

_bool CMonster_BigDog::Face_Target(_fvector vTargetPos, _float fTimeDelta, _bool bAway)
{
	if (nullptr == m_pTransformCom || nullptr == m_pMovementCom)
		return false;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
		return true;

	vTo = XMVector3Normalize(vTo);

	_vector vGoal = bAway ? XMVectorSubtract(vMyPos, vTo) : XMVectorAdd(vMyPos, vTo);

	if (true == Can_AnimRotate())
		m_pMovementCom->TurnTo(vGoal, fTimeDelta);

	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
	const _float fDot = XMVectorGetX(XMVector3Dot(vLook, vTo));
	const _float fThresh = cosf(XMConvertToRadians(20.f));

	return bAway ? (fDot <= -fThresh) : (fDot >= fThresh);
}

_bool CMonster_BigDog::Drive_Facing(_fvector vTargetPos, _float fTimeDelta, _bool bAway)
{
	if (nullptr == m_pTransformCom || nullptr == m_pMovementCom)
		return false;

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
	{
		m_bTurning = false;
		Set_AnimBool(L"Turn", false);
		return true;
	}

	vTo = XMVector3Normalize(vTo);
	_vector vDesired = bAway ? XMVectorNegate(vTo) : vTo;
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	_float fDot = XMVectorGetX(XMVector3Dot(vLook, vDesired));
	if (fDot > 1.f) fDot = 1.f;
	else if (fDot < -1.f) fDot = -1.f;
	const _float fAngleDeg = XMConvertToDegrees(acosf(fDot));

	if (false == m_bTurning)
	{
		if (fAngleDeg > m_fTurnStartDeg)
		{
			m_bTurning = true;
			const _float fCrossY = XMVectorGetY(XMVector3Cross(vLook, vDesired));
			m_iTurnDir = static_cast<_int>(fCrossY >= 0.f ? MON_DIRECTION::RIGHT : MON_DIRECTION::LEFT);
		}
	}
	else
	{
		if (fAngleDeg <= m_fTurnStopDeg)
			m_bTurning = false;
	}

	if (true == m_bTurning)
	{
		Set_AnimInt(L"Direction", m_iTurnDir);
		Set_AnimBool(L"Turn", true);
	}
	else
	{
		Set_AnimBool(L"Turn", false);
	}

	if (true == Can_AnimRotate())
	{
		_vector vGoal = XMVectorAdd(vMyPos, vDesired);
		m_pMovementCom->TurnTo(vGoal, fTimeDelta);
	}

	const _float fThresh = cosf(XMConvertToRadians(20.f));
	return (fDot >= fThresh);
}

void CMonster_BigDog::Drive_Steering_BigDog(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);
	Set_AnimBool(L"Turn", false);

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") ||
		m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bDead"))
		return;

	if (false == m_bSpawned || false == m_bAggro || nullptr == m_pMovementCom)
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	const _int iActive = (m_iPendingPatternID >= 0) ? m_iPendingPatternID : m_iCurPatternID;
	const _bool bRear = Is_RearPattern(iActive);

	const ANIM_CLIP_SETTING* pCurClip = (nullptr != m_pAnimator) ? m_pAnimator->Get_CurClip() : nullptr;
	const _bool bRootClip = (nullptr != pCurClip) && pCurClip->bUseRootMotion;
	const _bool bSkillMove = (nullptr != m_pAnimator) && m_pAnimator->Is_InCustomWindow(L"SkillMove_Move");

	const wchar_t* szMode = L"NONE";

	if (true == bSkillMove)
	{
		szMode = L"SKILLMOVE";
		if (false == bRear)
			Face_Target(vTargetPos, fTimeDelta, false);
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
		m_pMovementCom->Set_HomingSpeed(m_fSkillMoveSpeed);
		m_pMovementCom->Set_MoveTarget(vTargetPos);
	}
	else if (true == bRootClip)
	{
		if (true == Is_HitActive())
		{
			szMode = L"ATTACK_HITLOCK";
		}
		else
		{
			szMode = bRear ? L"ATTACK_TRACK_BACK" : L"ATTACK_TRACK";
			Face_Target(vTargetPos, fTimeDelta, bRear);
		}
	}
	else
	{
		const _bool bInAttackRange = m_pBlackBoard->Get<_bool>(L"bInAttackRange");

		const _bool bApproaching = (false == bInAttackRange)
			|| (m_iPendingPatternID >= 0 && m_fDistToTarget > m_fPendingAttackLength + m_fAttackRangeTol);

		if (true == m_bMoving && true == Can_AnimMove())
		{
			szMode = (m_iMoveDir == static_cast<_int>(MON_DIRECTION::LEFT)) ? L"STRAFE_L" : L"STRAFE_R";
			Face_Target(vTargetPos, fTimeDelta, false);
			Set_AnimInt(L"Direction", m_iMoveDir);
			Set_AnimFloat(L"Speed", 1.f);
			m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
			m_pMovementCom->Orbit(m_iMoveDir == static_cast<_int>(MON_DIRECTION::LEFT) ? 0 : 1, vTargetPos, fTimeDelta);
		}
		else if (true == bApproaching)
		{
			szMode = L"APPROACH";
			if (true == Can_AnimMove())
			{
				const _bool bFar = Wants_RunChase(m_fDistToTarget);
				Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
				Set_AnimFloat(L"Speed", bFar ? 3.f : 1.f);
				m_pMovementCom->Set_SpeedType(bFar ? SPEED_TYPE::SPRINT : SPEED_TYPE::RUN);
				m_pMovementCom->MoveTo_Navi(vTargetPos, fTimeDelta);
			}
		}
		else
		{
			szMode = m_bRearSnapshot ? L"FACE_BACK" : L"FACE";
			Drive_Facing(vTargetPos, fTimeDelta, m_bRearSnapshot);
		}
	}

	static _float s_fSteerLogT = 0.f;
	s_fSteerLogT += fTimeDelta;
	if (s_fSteerLogT >= 0.2f)
	{
		s_fSteerLogT = 0.f;
		wchar_t szBuf[256];
		swprintf_s(szBuf, L"[BigDog][STEER] mode=%ls rear=%d rearSnap=%d rootClip=%d hitActive=%d speed=%.2f dist=%.2f\n",
			szMode, (_int)bRear, (_int)m_bRearSnapshot, (_int)bRootClip, (_int)Is_HitActive(),
			m_pMovementCom->Get_CurrentSpeed(), m_fDistToTarget);
		OutputDebugStringW(szBuf);
	}
}

void CMonster_BigDog::Update_PatternSelect(_float fTimeDelta)
{
	{
		static _float s_fHeartbeat = 0.f;
		s_fHeartbeat += fTimeDelta;
		if (s_fHeartbeat >= 0.2f)
		{
			s_fHeartbeat = 0.f;
			const _bool bIdleNow = Is_AnimIdle();
			const _bool bCanCancelNow = (nullptr != m_pAnimator && m_pAnimator->Can_Cancel());
			const ANIM_CONTROLLER_STATE* pState = (nullptr != m_pAnimator) ? m_pAnimator->Get_CurControllerState() : nullptr;
			wchar_t szBuf[384];
			swprintf_s(szBuf,
				L"[BigDog][HB] state=%hs dist=%.2f inRange=%d idle=%d idleT=%.2f canCancel=%d pend=%d cur=%d front=%d rearSnap=%d\n",
				(nullptr != pState) ? pState->strName.c_str() : "none",
				m_fDistToTarget,
				(_int)m_pBlackBoard->Get<_bool>(L"bInAttackRange"),
				(_int)bIdleNow, m_fIdleTimer,
				(_int)bCanCancelNow,
				m_iPendingPatternID, m_iCurPatternID,
				(_int)Is_TargetInFront(), (_int)m_bRearSnapshot);
			OutputDebugStringW(szBuf);
		}
	}

	if (m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bGroggy") ||
		m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bDead"))
	{
		m_iPendingPatternID = -1;
		m_bApproaching = false;
		m_bWasIdle = false;
		m_bMoving = false;
		m_bMustAttack = false;
		return;
	}

	if (false == m_pBlackBoard->Get<_bool>(L"bInAttackRange"))
	{
		m_iPendingPatternID = -1;
		m_bApproaching = false;
		m_bWasIdle = false;
		m_bMoving = false;
		m_bMustAttack = false;
		return;
	}

	if (m_iPendingPatternID >= 0)
	{
		if (REAR_MODE::CONTEXTUAL == m_eRearMode)
		{
			const _bool bPendRear = Is_RearPattern(m_iPendingPatternID);
			if ((true == bPendRear && true == Is_TargetInFront())
				|| (false == bPendRear && true == Is_TargetInRear()))
			{
				m_iPendingPatternID = -1;
				m_bApproaching = false;
				m_bRearSnapshot = Is_TargetInRear();
				if (m_pMovementCom) m_pMovementCom->Stop_MoveTo();
				return;
			}
		}

		if (m_fDistToTarget > m_fPendingAttackLength + m_fAttackRangeTol)
		{
			m_bApproaching = true;
			return;
		}
		m_bApproaching = false;

		if (true == Is_RearPattern(m_iPendingPatternID) && false == Is_BackAligned())
			return;

		{
			wchar_t szBuf[192];
			swprintf_s(szBuf, L"[BigDog][EVT] FIRE pattern=%d dist=%.2f rearSnap=%d\n",
				m_iPendingPatternID, m_fDistToTarget, (_int)m_bRearSnapshot);
			OutputDebugStringW(szBuf);
		}

		m_iCurPatternID = m_iPendingPatternID;
		m_iPendingPatternID = -1;
		if (m_pMovementCom) m_pMovementCom->Stop_MoveTo();

		Fire_Attack();
		return;
	}

	if (true == m_bMoving)
	{
		m_fMoveTimer -= fTimeDelta;
		if (m_fMoveTimer <= 0.f)
		{
			m_bMoving = false;
			m_bMustAttack = true;
		}
		return;
	}

	const _bool bIdle = Is_AnimIdle()
		|| Is_AnimInState(L"TURN_LEFT") || Is_AnimInState(L"TURN_RIGHT");
	if (true == bIdle && false == m_bWasIdle && m_iPendingPatternID < 0)
		m_bRearSnapshot = (REAR_MODE::CONTEXTUAL == m_eRearMode) ? Is_TargetInRear() : false;
	m_bWasIdle = bIdle;
	m_fIdleTimer = bIdle ? (m_fIdleTimer + fTimeDelta) : 0.f;

	const _bool bCanCancel = (nullptr != m_pAnimator && m_pAnimator->Can_Cancel());
	const _bool bIdleReady = (true == bIdle && m_fIdleTimer >= m_fAttackGap);
	const _bool bCancelReady = (true == bCanCancel && false == m_bCancelConsumed);

	if (false == bCanCancel)
		m_bCancelConsumed = false;

	if (false == bIdleReady && false == bCancelReady)
		return;

	if (true == bCancelReady)
		m_bCancelConsumed = true;

	if (true == Try_Begin_Intro())
		return;

	if (false == m_bMustAttack
		&& (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) < m_fMoveChance)
	{
		m_iMoveDir = (rand() % 2 == 0) ? static_cast<_int>(MON_DIRECTION::LEFT)
			: static_cast<_int>(MON_DIRECTION::RIGHT);
		m_bMoving = true;
		m_fMoveTimer = m_fMoveDuration;
		m_fIdleTimer = 0.f;

		wchar_t szBuf[128];
		swprintf_s(szBuf, L"[BigDog][EVT] STRAFE dir=%d dur=%.2f dist=%.2f\n",
			m_iMoveDir, m_fMoveDuration, m_fDistToTarget);
		OutputDebugStringW(szBuf);
		return;
	}
	m_bMustAttack = false;

	const _int iID = Select_Pattern_Angle(m_fDistToTarget);
	const _bool bRear = Is_RearPattern(iID);
	const _float fLen = bRear ? m_fRearTailRange : Get_PatternLength(iID);
	m_bRearSnapshot = bRear;

	{
		wchar_t szBuf[256];
		swprintf_s(szBuf, L"[BigDog][EVT] BEGIN pattern=%d len=%.1f dist=%.2f rear=%d\n",
			iID, fLen, m_fDistToTarget, (_int)bRear);
		OutputDebugStringW(szBuf);
	}

	Begin_Pattern(iID, fLen);
}

_int CMonster_BigDog::Select_Pattern_Angle(_float fDist)
{
	const _int iCount = m_iPatternDefCount;
	const _bool bContextual = (REAR_MODE::CONTEXTUAL == m_eRearMode);
	const _bool bTargetRear = Is_TargetInRear();

	_float fW[16] = {};
	_float fTotal = 0.f;
	for (_int i = 0; i < iCount; ++i)
	{
		const _bool bRear = Is_RearPattern(s_Patterns[i].iID);

		if (true == bContextual && bRear != bTargetRear)
		{
			fW[i] = 0.f;
			continue;
		}

		const _float fLen = bRear ? m_fRearTailRange : s_Patterns[i].fAttackLength;
		const _float fDiff = fabsf(fDist - fLen);
		_float w = s_Patterns[i].fWeight / (1.f + fDiff);
		if (s_Patterns[i].iID == m_iLastPatternID)
			w *= 0.15f;
		fW[i] = w;
		fTotal += w;
	}

	if (fTotal <= 0.f)
	{
		for (_int i = 0; i < iCount; ++i)
			if (Is_RearPattern(s_Patterns[i].iID) == bTargetRear)
			{
				m_iLastPatternID = s_Patterns[i].iID;
				return s_Patterns[i].iID;
			}
		return s_Patterns[0].iID;
	}

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	_int iPick = 0;
	for (_int i = 0; i < iCount; ++i)
	{
		r -= fW[i];
		if (r <= 0.f) { iPick = i; break; }
	}
	m_iLastPatternID = s_Patterns[iPick].iID;
	return s_Patterns[iPick].iID;
}

HRESULT CMonster_BigDog::Ready_HitBoxes()
{
	m_HitBoxes.Add_BoneSphere(L"HandL", "Bip001-L-Hand", 0.8f, 900.f);
	m_HitBoxes.Add_BoneSphere(L"HandR", "Bip001-R-Hand", 0.8f, 900.f);
	m_HitBoxes.Add_BoneSphere(L"Head", "Bone001_Tongue_01", 1.0f, 1200.f);
	m_HitBoxes.Add_BoneSphere(L"Body", "Bip001-Spine1", 2.0f, 1300.f);
	m_HitBoxes.Add_BoneSphere(L"ForearmL", "Bip001-L-Forearm", 0.9f, 1000.f);
	m_HitBoxes.Add_BoneSphere(L"ForearmR", "Bip001-R-Forearm", 0.9f, 1000.f);

	m_HitBoxes.Add_BoneSphere(L"Tail", "SD_SK_Blade", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_09", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_08", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_07", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_06", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_05", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_04", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_03", 0.35f, 800.f);
	m_HitBoxes.Add_BoneSphere(L"Tail", "Bone001_Tail01_02", 0.35f, 800.f);

	if (FAILED(m_HitBoxes.Build(m_pModelCom, m_pDevice, m_pContext)))
		return E_FAIL;

	return S_OK;
}

void CMonster_BigDog::On_Hit_Post(const HitInfo& hitInfo)
{
	Wake_And_FaceHit(hitInfo);
}

_bool CMonster_BigDog::Blocks_Damage_While_Groggy() const
{
	return true;
}

_bool CMonster_BigDog::Uses_Full_Hit_Reaction() const
{
	return false;
}

_float CMonster_BigDog::Get_AttackRange() const
{
	return m_fFarAttackRange;
}

_bool CMonster_BigDog::Wants_RunChase(_float fDist) const
{
	return fDist > m_fRunChaseRange;
}

CMonster_BigDog* CMonster_BigDog::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_BigDog* pInstance = new CMonster_BigDog(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_BigDog");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_BigDog::Clone(void* pArg)
{
	CMonster_BigDog* pInstance = new CMonster_BigDog(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_BigDog");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_BigDog::Free()
{
	m_HitBoxes.Free();

	__super::Free();
}

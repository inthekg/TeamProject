#include "Monster_Priest.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"
#include "Model.h"
#include "Weapon_Melee.h"
#include "Monster_Attach.h"
#include "Animator.h"
#include "ElectricBall.h"
#include "Effect.h"
#include "StatComponent_AI.h"
#include "Movement_AI.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_Priest, CMonster_Melee)
END_REFLECTION(CMonster_Priest)
#endif

namespace
{
	const PATTERN_DEF s_Patterns[] =
	{
		{ 1, ATK_GROUP::LONG,  1.f, 10.0f },
		{ 2, ATK_GROUP::SHORT, 1.f,  4.5f },
		{ 3, ATK_GROUP::SHORT, 1.f,  3.3f },
		{ 4, ATK_GROUP::SHORT, 1.f,  4.5f },
		{ 5, ATK_GROUP::LONG,  1.f,  6.0f },
	};

	const _int DBG_PATTERN_COUNT = 5;

	typedef struct tagPriestLink
	{
		_int	iFrom;
		_uint	iStartFrame;
		_int	iTo;
		_float	fDistMax;
		_float	fHalfAngleDeg;
	} PRIEST_LINK;

	const PRIEST_LINK s_Links[] =
	{
		{   2,  76, 101, 4.0f,  80.f },
		{ 101,  61, 102, 4.5f,  80.f },
		{   3, 107, 103, 4.5f,  90.f },
		{   4,  66, 104, 5.0f, 180.f },
		{ 104,  71, 105, 5.0f,  90.f },
		{ 106,  50, 107, 5.0f,  90.f },
		{ 106,  79, 105, 0.0f,   0.f },
	};

	const _int	LINK_FRONTCHECK_FROM = 5;
	const _int	LINK_FRONTCHECK_TO = 106;

	typedef struct tagPriestPick
	{
		_int	iID;
		_float	fWeight;
		_float	fBandMin;
		_float	fBandMax;
	} PRIEST_PICK;

	const PRIEST_PICK s_Picks[] =
	{
		{ 1, 1.0f, 5.0f, 99.0f },
		{ 2, 1.2f, 0.0f,  5.5f },
		{ 3, 1.0f, 0.0f,  4.5f },
		{ 4, 1.2f, 0.0f,  5.5f },
		{ 5, 1.0f, 3.0f,  8.0f },
	};

	const _int PICK_COUNT = static_cast<_int>(sizeof(s_Picks) / sizeof(PRIEST_PICK));
}

CMonster_Priest::CMonster_Priest(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Melee{ pDevice, pContext }
{
}

CMonster_Priest::CMonster_Priest(const CMonster_Priest& Prototype)
	: CMonster_Melee{ Prototype }
{
}

HRESULT CMonster_Priest::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Priest.json";

	m_pPatterns = s_Patterns;
	m_iPatternDefCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	if (FAILED(Ready_AttackData()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Priest::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Priest");
		return E_FAIL;
	}

	MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);

	Apply_MeleeDesc(*pDesc);
	m_fFarAttackRange = pDesc->fFarAttackRange;
	m_fRunChaseRange = pDesc->fRunChaseRange;
	Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	CHARACTER_SETUP_DESC CharDesc{};
	CharDesc.strModelPrototypeTag = pDesc->strModelProtoTag;
	CharDesc.strAnimFolderTag = pDesc->strAnimFolderTag;
	CharDesc.strAnimTag = pDesc->strAnimFolderTag;
	CharDesc.pRootBoneName = pDesc->pRootBoneName;
	CharDesc.bUseCCT = true;
	CharDesc.iModelLevel = pDesc->iProtoLevel;

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_Weapon(*pDesc)))
		return E_FAIL;

	if (FAILED(Ready_Attachments(*pDesc)))
		return E_FAIL;

	Set_IntroPattern(-1);

	Spawn();

	return S_OK;
}

HRESULT CMonster_Priest::Ready_Attachments(const MONSTER_DESC& Desc)
{
	CMonster_Attach::ATTACH_DESC ad{};
	ad.pOwner = this;
	ad.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	ad.iLevelID = ETOUI(LEVEL::GAMEPLAY);
	ad.bRender = true;
	ad.bShadow = true;

	if (false == Desc.strCanisterProtoTag.empty())
	{
		ad.strModelTag = Desc.strCanisterProtoTag;
		ad.strSocketBoneName = Desc.strCanisterBone;

		if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
			TEXT("Prototype_GameObject_Monster_Attach"), TEXT("Obj_Canister"), &ad)))
			return E_FAIL;

		m_pCanister = static_cast<CMonster_Attach*>(Get_PartObject(TEXT("Obj_Canister")));
	}

	if (false == Desc.strCapProtoTag.empty())
	{
		ad.strModelTag = Desc.strCapProtoTag;
		ad.strSocketBoneName = Desc.strCapBone;

		if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
			TEXT("Prototype_GameObject_Monster_Attach"), TEXT("Obj_Cap"), &ad)))
			return E_FAIL;

		m_pCap = static_cast<CMonster_Attach*>(Get_PartObject(TEXT("Obj_Cap")));
	}

	return S_OK;
}

HRESULT CMonster_Priest::Ready_NotifyKeys()
{
	if (FAILED(__super::Ready_NotifyKeys()))
		return E_FAIL;

	m_NotifyKeys[Hash_String(L"Fire_ElectricBall")] = NOTIFY_KEY::FIRE;
	return S_OK;
}

HRESULT CMonster_Priest::Ready_AttackData()
{
	m_AttackData[Hash_String(L"ATTACK_01")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_02")] = { 600.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_03")] = { 1000.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_04")] = { 600.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_05")] = { 1000.f, HIT_TYPE::Normal };

	m_AttackData[Hash_String(L"COMBO_01")] = { 700.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_02")] = { 1000.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_03")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_04")] = { 700.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_05")] = { 900.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_06")] = { 800.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"COMBO_07")] = { 1000.f, HIT_TYPE::Normal };
	return S_OK;
}

void CMonster_Priest::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
		Apply_Movment(fTimeDelta);
}

void CMonster_Priest::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Priest::Late_Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	if (nullptr != m_pCanister)
		m_pCanister->Late_Update(fTimeDelta);

	if (nullptr != m_pCap)
		m_pCap->Late_Update(fTimeDelta);

#ifdef _DEBUG
	Debug_MeasureAttack(fTimeDelta);
#endif
}

HRESULT CMonster_Priest::Render()
{
	return __super::Render();
}

void CMonster_Priest::Spawn()
{
	m_pWeaponFx = m_pGameInstance->Play_Effect_Model("Effect_Priest_Weapon_Thunder", m_pWeapon->Get_Model(), m_pWeapon->Get_CombinedWorldMatrixPtr());
	__super::Spawn();

	m_bChasing = true;
	m_fChaseSwitchTimer = 0.f;
	m_bTurning = false;
	m_iTurnDir = static_cast<_int>(MON_DIRECTION::FRONT);

	m_bBallAiming = false;
	m_bBallFired = false;
	m_fBallPrevRotSpeed = 0.f;

	m_bParryReact = false;
	m_bLinkFired = false;
	m_iLinkOwnerID = -1;
}

void CMonster_Priest::Despawn()
{
	m_pWeaponFx->Stop();
	__super::Despawn();
}

void CMonster_Priest::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	Update_CombatParams(fTimeDelta);
	Update_ChaseHysteresis(fTimeDelta);
	Update_Activation();

#ifdef _DEBUG
	if (true == m_bDbgMeasuring)
	{
		m_pBlackBoard->Set<_bool>(L"bAttackReady", false);
		Set_AnimInt(L"PatternID", m_iDbgAttackID);
		m_fIdleTimer = 0.f;
		m_bMoving = false;
		Drive_Steering_Priest(fTimeDelta);
		Mirror_ControllerParams();
		return;
	}
#endif

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
			Is_AnimInState(L"DEAD_B") || Is_AnimInState(L"DEAD_F") ||
			Is_AnimInState(L"FATAL_DEAD_B") || Is_AnimInState(L"FATAL_DEAD_F");
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

	if (false == m_bSpawned)
	{
		const _bool bFrontDetect =
			m_pBlackBoard->Get<_bool>(L"bPlayerInDetect") && m_pBlackBoard->Get<_bool>(L"bPlayerInFront");

		if (true == bFrontDetect)
			m_bSpawned = true;

		if (false == Is_AnimInState(L"SPAWN"))
			Request_AnimAction(L"SPAWN");

		Set_AnimFloat(L"Speed", 0.f);
		Mirror_ControllerParams();
		return;
	}

	if (m_pBlackBoard->Get<_bool>(L"bHit"))
	{
		const _bool bFreshHit = Consume_HitRefresh();

		if (false == m_bParryReact)
		{
			m_pBlackBoard->Set<_bool>(L"bHit", false);
		}
		else
		{
			m_fHitTimer += fTimeDelta;
			if (true == bFreshHit)
				m_fHitTimer = 0.f;

			const _bool bInReact =
				Is_AnimInState(L"HIT_B") || Is_AnimInState(L"HIT_L") || Is_AnimInState(L"HIT_R");
			if (false == bInReact || true == bFreshHit)
				Fire_Knockback();

			if (m_fHitTimer >= m_fHitMaxTime)
			{
				m_pBlackBoard->Set<_bool>(L"bHit", false);
				m_fHitTimer = 0.f;
				m_bParryReact = false;
			}

			m_bMoving = false;
			m_bMustAttack = false;
			Mirror_ControllerParams();
			return;
		}
	}
	m_fHitTimer = 0.f;

	_vector vSpawn = Get_SpawnPos();
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

	Update_ComboLink();
	Update_PatternSelect(fTimeDelta);
	Update_BallAim(fTimeDelta);
	Drive_Steering_Priest(fTimeDelta);
	Drive_MoveToTarget(fTimeDelta);
	Mirror_ControllerParams();
}

void CMonster_Priest::Update_ChaseHysteresis(_float fTimeDelta)
{
	const _float fReach = Get_AttackRange();

	m_fChaseSwitchTimer += fTimeDelta;
	if (m_fChaseSwitchTimer >= m_fChaseDwell)
	{
		if (false == m_bChasing && m_fDistToTarget > fReach + m_fChaseMargin)
		{
			m_bChasing = true;
			m_fChaseSwitchTimer = 0.f;
		}
		else if (true == m_bChasing && m_fDistToTarget <= fReach)
		{
			m_bChasing = false;
			m_fChaseSwitchTimer = 0.f;
		}
	}

	m_pBlackBoard->Set<_bool>(L"bInAttackRange", m_bAggro && false == m_bChasing);
}

void CMonster_Priest::Drive_Facing_Priest(_fvector vTargetPos, _float fTimeDelta)
{
	if (nullptr == m_pTransformCom || nullptr == m_pMovementCom)
		return;

	if (true == m_bBallAiming)
	{
		m_bTurning = false;
		return;
	}

	_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	_vector vTo = XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTo)) < 1e-6f)
	{
		m_bTurning = false;
		return;
	}

	vTo = XMVector3Normalize(vTo);
	_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));

	_float fDot = XMVectorGetX(XMVector3Dot(vLook, vTo));
	if (fDot > 1.f) fDot = 1.f;
	else if (fDot < -1.f) fDot = -1.f;
	const _float fAngleDeg = XMConvertToDegrees(acosf(fDot));

	if (false == m_bTurning)
	{
		if (fAngleDeg > m_fTurnStartDeg)
		{
			m_bTurning = true;
			const _float fCrossY = XMVectorGetY(XMVector3Cross(vLook, vTo));
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

	if (true == Can_AnimRotate())
		m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
}

void CMonster_Priest::Drive_Steering_Priest(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);
	Set_AnimBool(L"Turn", false);

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") ||
		m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bDead"))
	{
		m_bTurning = false;
		return;
	}

	if (false == m_bSpawned || false == m_bAggro || nullptr == m_pMovementCom)
	{
		m_bTurning = false;
		return;
	}

	const _bool bInAttackRange = m_pBlackBoard->Get<_bool>(L"bInAttackRange");

	_vector vTargetPos;
	const _bool bHasTarget = Get_TargetPos(vTargetPos);

	if (false == bInAttackRange)
	{
		m_bTurning = false;

		if (true == bHasTarget && true == Can_AnimMove())
		{
			const _bool bFar = Wants_RunChase(m_fDistToTarget);
			Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
			Set_AnimFloat(L"Speed", bFar ? 3.f : 1.f);
			m_pMovementCom->Set_SpeedType(bFar ? SPEED_TYPE::SPRINT : SPEED_TYPE::RUN);
			m_pMovementCom->MoveTo_Navi(vTargetPos, fTimeDelta);
		}
		return;
	}

	if (true == bHasTarget)
		Drive_Facing_Priest(vTargetPos, fTimeDelta);
	else
		m_bTurning = false;

	if (true == m_bTurning)
		return;

	if (true == m_bApproaching && true == bHasTarget && true == Can_AnimMove())
	{
		Set_AnimInt(L"Direction", static_cast<_int>(MON_DIRECTION::FRONT));
		Set_AnimFloat(L"Speed", 1.f);
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
		m_pMovementCom->Set_MoveTarget(vTargetPos);
		return;
	}

	if (true == m_bMoving && true == Can_AnimMove())
	{
		Set_AnimInt(L"Direction", m_iMoveDir);
		Set_AnimFloat(L"Speed", 1.f);
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::RUN);
		switch (static_cast<MON_DIRECTION>(m_iMoveDir))
		{
		case MON_DIRECTION::LEFT:  m_pMovementCom->Orbit(0, vTargetPos, fTimeDelta); break;
		case MON_DIRECTION::RIGHT: m_pMovementCom->Orbit(1, vTargetPos, fTimeDelta); break;
		default: break;
		}
	}
}

void CMonster_Priest::Update_ComboLink()
{
	if (nullptr == m_pAnimator)
		return;

	if (m_iCurPatternID != m_iLinkOwnerID)
	{
		m_iLinkOwnerID = m_iCurPatternID;
		m_bLinkFired = false;
	}

	if (true == m_bLinkFired)
		return;

	if (m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bGroggy") ||
		m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bDead"))
		return;

	if (LINK_FRONTCHECK_FROM == m_iCurPatternID)
	{
		if (false == m_pAnimator->Is_InCustomWindow(g_strNearByWindow))
			return;

		if (m_fDistToTarget > m_fLinkFrontRange)
			return;

		if (false == Is_FacingTarget(cosf(XMConvertToRadians(m_fLinkFrontDeg))))
			return;

		Fire_ComboLink(LINK_FRONTCHECK_TO);
		return;
	}

	static const _uint64 iLinkHash = Hash_String(L"Link");

	for (const ANIM_WINDOW* pWindow : m_pAnimator->Get_ActiveCustomWindows())
	{
		if (nullptr == pWindow || iLinkHash != pWindow->iPayloadHash)
			continue;

		for (const PRIEST_LINK& Link : s_Links)
		{
			if (Link.iFrom != m_iCurPatternID || Link.iStartFrame != pWindow->iStartFrame)
				continue;

			if (Link.fDistMax > 0.f)
			{
				if (m_fDistToTarget > Link.fDistMax)
					return;

				if (Link.fHalfAngleDeg < 180.f &&
					false == Is_FacingTarget(cosf(XMConvertToRadians(Link.fHalfAngleDeg))))
					return;
			}

			if (pWindow->fValue < 0.5f &&
				(nullptr == m_pWeapon || false == m_pWeapon->Is_HitSuccess()))
				return;

			Fire_ComboLink(Link.iTo);
			return;
		}
	}
}

void CMonster_Priest::Fire_ComboLink(_int iPatternID)
{
	m_bLinkFired = true;
	m_iLastPatternID = m_iCurPatternID;
	m_iCurPatternID = iPatternID;

	m_iPendingPatternID = -1;
	m_bApproaching = false;
	m_bMoving = false;
	m_bMustAttack = false;
	m_fIdleTimer = 0.f;

	Set_AnimInt(L"PatternID", iPatternID);
	Set_AnimTrigger(L"PatternStart");
	Request_AnimAction(L"ATTACK");

#ifdef _DEBUG
	_tchar szLog[160];
	swprintf_s(szLog, L"[PriestLink] %d -> %d  dist=%.2f\n", m_iLastPatternID, iPatternID, m_fDistToTarget);
	OutputDebugStringW(szLog);
#endif
}

void CMonster_Priest::Update_PatternSelect(_float fTimeDelta)
{
	if (false == m_bSpawned)
		return;

	if (m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bGroggy") ||
		m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bDead"))
	{
		m_bMoving = false;
		m_bMustAttack = false;
		m_iPendingPatternID = -1;
		m_bApproaching = false;
		return;
	}

	if (false == m_pBlackBoard->Get<_bool>(L"bInAttackRange"))
	{
		m_bMoving = false;
		m_iPendingPatternID = -1;
		m_bApproaching = false;
		return;
	}

	if (m_iPendingPatternID >= 0)
	{
		if (m_fDistToTarget > m_fPendingAttackLength + m_fAttackRangeTol)
		{
			m_bApproaching = true;
			return;
		}
		m_bApproaching = false;
		m_iCurPatternID = m_iPendingPatternID;
		m_iPendingPatternID = -1;
		m_iLinkOwnerID = m_iCurPatternID;
		m_bLinkFired = false;
		Fire_Attack();
		return;
	}

	if (true == m_bMoving)
	{
		m_fMoveTimer -= fTimeDelta;
		if (m_fMoveTimer <= 0.f) { m_bMoving = false; m_bMustAttack = true; }
		return;
	}

	const _bool bIdle = Is_AnimIdle();
	m_fIdleTimer = bIdle ? (m_fIdleTimer + fTimeDelta) : 0.f;

	if (false == bIdle || m_fIdleTimer < m_fAttackGap)
		return;

	const _int iAct = Pick_Action_Priest(m_fDistToTarget, false == m_bMustAttack);

	if (iAct < PICK_COUNT)
	{
		m_bMustAttack = false;
		m_iLastPatternID = s_Picks[iAct].iID;
		Begin_Pattern(s_Picks[iAct].iID, Get_PatternLength(s_Picks[iAct].iID));
		return;
	}

	const _int iDir = rand() % 2;
	m_iMoveDir = (0 == iDir) ? static_cast<_int>(MON_DIRECTION::LEFT)
		: static_cast<_int>(MON_DIRECTION::RIGHT);
	m_bMoving = true;
	m_fMoveTimer = m_fMoveDuration;
	m_fIdleTimer = 0.f;
}

_int CMonster_Priest::Pick_Action_Priest(_float fDist, _bool bAllowMove)
{
	_float fW[PICK_COUNT + 1] = {};
	_float fTotal = 0.f;

	for (_int i = 0; i < PICK_COUNT; ++i)
	{
		_float w = s_Picks[i].fWeight;

		if (fDist < s_Picks[i].fBandMin || fDist > s_Picks[i].fBandMax)
			w *= m_fOffBandScale;

		if (s_Picks[i].iID == m_iLastPatternID)
			w *= m_fRepeatPenalty;

		fW[i] = w;
		fTotal += w;
	}

	if (fTotal <= 0.f)
		return PICK_COUNT;

	if (true == bAllowMove)
	{
		fW[PICK_COUNT] = m_fMoveWeight;
		fTotal += m_fMoveWeight;
	}

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	_int iPick = PICK_COUNT;
	for (_int i = 0; i <= PICK_COUNT; ++i)
	{
		r -= fW[i];
		if (r <= 0.f) { iPick = i; break; }
	}

#ifdef _DEBUG
	_tchar szLog[256];
	_tchar szW[160] = {};
	for (_int i = 0; i < PICK_COUNT; ++i)
	{
		_tchar szOne[32];
		swprintf_s(szOne, L"P%d %.0f%%  ", s_Picks[i].iID, 100.f * fW[i] / fTotal);
		wcscat_s(szW, szOne);
	}
	swprintf_s(szLog, L"[PriestPick] dist=%.2f  %sMOVE %.0f%%  -> %s\n",
		fDist, szW, 100.f * fW[PICK_COUNT] / fTotal,
		(iPick < PICK_COUNT) ? L"ATTACK" : L"MOVE");
	OutputDebugStringW(szLog);
#endif

	return iPick;
}

void CMonster_Priest::Action_Notify(_uint64 iHashKey)
{
	static const _uint64 iFireKey = Hash_String(L"Fire_ElectricBall");

	if (iHashKey == iFireKey)
	{
		Fire_ElectricBall();
		return;
	}

	__super::Action_Notify(iHashKey);
}

void CMonster_Priest::Update_BallAim(_float fTimeDelta)
{
	if (nullptr == m_pTransformCom || nullptr == m_pMovementCom)
		return;

	const _bool bInBallState = Is_AnimInState(L"ATTACK_01");
	const _bool bCasting = (true == bInBallState) && (false == m_bBallFired);

	if (false == bCasting)
	{
		if (true == m_bBallAiming)
		{
			m_pTransformCom->Set_RotationPerSec(m_fBallPrevRotSpeed);
			m_bBallAiming = false;
		}

		if (false == bInBallState)
			m_bBallFired = false;

		return;
	}

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	if (false == m_bBallAiming)
	{
		m_fBallPrevRotSpeed = m_pTransformCom->Get_RotationPerSec();
		m_bBallAiming = true;
	}

	m_pTransformCom->Set_RotationPerSec(XMConvertToRadians(m_fBallTrackSpeed));
	m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
}

void CMonster_Priest::Fire_ElectricBall()
{
	if (nullptr == m_pTransformCom)
		return;

	m_bBallFired = true;

	const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);

	_vector vTargetPos;
	const _bool bHasTarget = Get_TargetPos(vTargetPos);

	if (true == bHasTarget)
		m_pTransformCom->LookAt(XMVectorSetY(vTargetPos, XMVectorGetY(vMyPos)));

	_vector vLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f)
		return;
	vLook = XMVector3Normalize(vLook);

	_vector vOrigin = XMVectorAdd(vMyPos, XMVectorScale(vLook, m_fBallForward));
	vOrigin = XMVectorSetY(vOrigin, XMVectorGetY(vMyPos) + m_fBallHeight);

	_matrix matHandle;
	if (nullptr != m_pWeapon && true == m_pWeapon->Get_BoneWorld(m_strBallBone, matHandle))
		vOrigin = matHandle.r[3];

	_vector vAim = vLook;

	if (true == bHasTarget)
	{
		_vector vAimPos = XMVectorSetY(vTargetPos, XMVectorGetY(vTargetPos) + m_fBallAimHeight);

		_vector vTo = XMVectorSubtract(vAimPos, vOrigin);
		if (XMVectorGetX(XMVector3LengthSq(vTo)) > 1e-6f)
			vAim = XMVector3Normalize(vTo);
	}

	CProjectile::FIRE_DESC fd{};
	fd.eMode = CProjectile::MODE::SHOT;
	XMStoreFloat3(&fd.vOrigin, vOrigin);
	XMStoreFloat3(&fd.vDir, vAim);
	fd.vScale = _float3(0.1f, 0.1f, 0.1f);  
	fd.fSpeed = m_fBallSpeed;
	fd.fAccel = 5.f;
	fd.fRange = m_fBallRange;
	fd.fRadius = m_fBallRadius;
	fd.fDamage = m_fBallDamage;
	fd.eHitType = HIT_TYPE::Heavy;
	fd.pOwner = this;
	fd.pTarget = m_pTarget;

	CElectricBall* pProjectile = m_pGameInstance->Get_PoolObject<CElectricBall>(&fd);
	if (nullptr == pProjectile)
		return;

	m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()),
		TEXT("Layer_Projectile"), pProjectile);

#ifdef _DEBUG
	_float3 vO, vD;
	XMStoreFloat3(&vO, vOrigin);
	XMStoreFloat3(&vD, vAim);
	_tchar szLog[192];
	swprintf_s(szLog, L"[Priest] Fire_ElectricBall  origin=(%.2f, %.2f, %.2f)  dir=(%.2f, %.2f, %.2f)\n",
		vO.x, vO.y, vO.z, vD.x, vD.y, vD.z);
	OutputDebugStringW(szLog);
#endif
}

void CMonster_Priest::On_CheckDead()
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

void CMonster_Priest::On_Parried()
{
	if (true == m_bDead || m_pStatCom->Is_Dead())
		return;

	if (m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bGroggy"))
		return;

	m_bParryReact = true;
	m_fHitTimer = 0.f;

	m_bMoving = false;
	m_bMustAttack = false;
	m_bApproaching = false;
	m_iPendingPatternID = -1;
	m_bLinkFired = true;

	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();

	m_pBlackBoard->Set<_bool>(L"bHit", true);

	Fire_Knockback();
}

void CMonster_Priest::Set_FatalMode(_bool bMode)
{
	__super::Set_FatalMode(bMode);
}

void CMonster_Priest::On_Hit_Post(const HitInfo& hitInfo)
{
	Wake_And_FaceHit(hitInfo);
}

_bool CMonster_Priest::Blocks_Damage_While_Groggy() const
{
	return false;
}

_float CMonster_Priest::Get_AttackRange() const
{
	return m_fFarAttackRange;
}

_bool CMonster_Priest::Wants_RunChase(_float fDist) const
{
	return fDist > m_fRunChaseRange;
}

const _wstring& CMonster_Priest::Get_KnockbackAction(_int iHitInfo) const
{
	static const _wstring strDamage = L"DAMAGE";
	return strDamage;
}

#ifdef _DEBUG
void CMonster_Priest::Debug_ResetToOrigin()
{
	const _vector vOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	if (nullptr != m_pCCT)
		m_pCCT->Set_FootPosition(vOrigin);

	if (nullptr != m_pTransformCom)
		m_pTransformCom->Set_State(STATE::POSITION, vOrigin);

	Cancel_All_Attacks();

	m_bDbgMeasuring = false;
	m_bDbgStarted = false;
	m_iDbgFireDelay = 0;
	m_bMoving = false;
	m_bMustAttack = false;
	m_iPendingPatternID = -1;
	m_bApproaching = false;

	OutputDebugStringW(L"[PriestRuler] reset to origin\n");
}

void CMonster_Priest::Debug_MeasureAttack(_float fTimeDelta)
{
	if (nullptr == m_pAnimator || nullptr == m_pTransformCom)
		return;

	if (true == m_pGameInstance->Key_Down(DIK_F1))
	{
		Debug_ResetToOrigin();
		return;
	}

	if (m_iDbgFireDelay > 0)
	{
		--m_iDbgFireDelay;

		if (m_iDbgFireDelay > 0)
			return;

		const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
		XMStoreFloat3(&m_vDbgStartPos, vMyPos);

		_vector vStartLook = XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vStartLook)) > 1e-6f)
			vStartLook = XMVector3Normalize(vStartLook);
		XMStoreFloat3(&m_vDbgStartLook, vStartLook);

		m_fDbgStartDist = -1.f;
		_vector vTargetPos;
		if (true == Get_TargetPos(vTargetPos))
			m_fDbgStartDist = XMVectorGetX(XMVector3Length(XMVectorSetY(XMVectorSubtract(vTargetPos, vMyPos), 0.f)));

		Set_AnimInt(L"PatternID", m_iDbgAttackID);
		Set_AnimTrigger(L"PatternStart");
		Request_AnimAction(L"ATTACK");

		_tchar szFire[160];
		swprintf_s(szFire, L"[PriestRuler] fire P%d  startDist=%.2f\n", m_iDbgAttackID, m_fDbgStartDist);
		OutputDebugStringW(szFire);
		return;
	}

	if (false == m_bDbgMeasuring)
	{
		static const _ubyte byFireKeys[DBG_PATTERN_COUNT] =
		{
			DIK_NUMPAD1, DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPAD4, DIK_NUMPAD5
		};

		for (_int i = 0; i < DBG_PATTERN_COUNT; ++i)
		{
			if (false == m_pGameInstance->Key_Down(byFireKeys[i]))
				continue;

			Cancel_All_Attacks();
			Begin_Battle();

			m_iDbgAttackID = i + 1;
			m_fDbgTimer = 0.f;
			m_fDbgScaleMax = 1.f;
			m_fDbgLunge = -9999.f;
			m_fDbgLungeAtHit = -9999.f;
			m_fDbgReach = -9999.f;
			m_bDbgStarted = false;
			m_bDbgMeasuring = true;
			m_iDbgFireDelay = 2;

			m_bMoving = false;
			m_bMustAttack = false;
			m_bApproaching = false;
			m_iPendingPatternID = -1;
			m_iCurPatternID = m_iDbgAttackID;
			break;
		}
		return;
	}

	m_fDbgTimer += fTimeDelta;
	m_bMoving = false;

	const _float fCurScale = Calc_RootMotionScale();
	if (fCurScale > m_fDbgScaleMax)
		m_fDbgScaleMax = fCurScale;

	const _vector vStart = XMLoadFloat3(&m_vDbgStartPos);
	const _vector vStartLook = XMLoadFloat3(&m_vDbgStartLook);

	const _vector vBody = m_pTransformCom->Get_State(STATE::POSITION);
	const _float  fLunge = XMVectorGetX(XMVector3Dot(XMVectorSetY(XMVectorSubtract(vBody, vStart), 0.f), vStartLook));
	if (fLunge > m_fDbgLunge)
		m_fDbgLunge = fLunge;

	if (false == Is_AnimIdle())
		m_bDbgStarted = true;

	if (true == Is_HitActive())
	{
		if (m_fDbgLungeAtHit < -9998.f)
			m_fDbgLungeAtHit = fLunge;

		if (nullptr != m_pWeapon)
		{
			_matrix matTop, matBot;
			if (SUCCEEDED(static_cast<CWeapon_Melee*>(m_pWeapon)->Get_TrailMatrix(matTop, matBot)))
			{
				const _vector vTips[2] = { matTop.r[3], matBot.r[3] };
				for (_int k = 0; k < 2; ++k)
				{
					const _float fFlat = XMVectorGetX(XMVector3Length(
						XMVectorSetY(XMVectorSubtract(vTips[k], vStart), 0.f)));
					if (fFlat + m_fSweepRadius > m_fDbgReach)
						m_fDbgReach = fFlat + m_fSweepRadius;
				}
			}
		}
	}

	const _bool bDone = (true == m_bDbgStarted && true == Is_AnimIdle());
	const _bool bTimeout = (m_fDbgTimer > 15.f);

	if (false == bDone && false == bTimeout)
		return;

	_tchar szLog[384];
	swprintf_s(szLog, L"[PriestRuler] P%d  scale=%.2f  startDist=%.2f  reach=%+.2f  lungeAtHit=%+.2f  lungeMax=%+.2f  t=%.2fs%s\n",
		m_iDbgAttackID, m_fDbgScaleMax, m_fDbgStartDist,
		m_fDbgReach, m_fDbgLungeAtHit, m_fDbgLunge, m_fDbgTimer,
		bTimeout ? L"  (TIMEOUT)" : L"");
	OutputDebugStringW(szLog);

	if (m_fDbgReach < -9998.f)
	{
		if (false == m_bDbgStarted)
			OutputDebugStringW(L"                 ATTACK_STATE_NOT_ENTERED (PatternID 조건 / ActionStateMaps 확인)\n");
		else
			OutputDebugStringW(L"                 NO_HITACTIVE (히트박스 없는 패턴이면 정상 - 발사체/버프)\n");
	}

	m_bDbgMeasuring = false;
}
#endif

CMonster_Priest* CMonster_Priest::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Priest* pInstance = new CMonster_Priest(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Priest");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Priest::Clone(void* pArg)
{
	CMonster_Priest* pInstance = new CMonster_Priest(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Priest");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Priest::Free()
{
	__super::Free();
}

#include "Monster_Elite.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"
#include "Model.h"
#include "Mesh.h"
#include "Weapon_Melee.h"
#include "Animator.h"

#include "StatComponent_AI.h"
#include "Movement_AI.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_Elite, CMonster_Melee)
END_REFLECTION(CMonster_Elite)
#endif

static const PATTERN_DEF s_Patterns[] =
{
	{ 1, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 2, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 3, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 4, ATK_GROUP::LONG,  1.f, 10.f },
	{ 5, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 6, ATK_GROUP::LONG,  1.f, 7.f },
	{ 7, ATK_GROUP::SHORT, 1.f, 1.5f },
};

CMonster_Elite::CMonster_Elite(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Melee{ pDevice, pContext }
{
}

CMonster_Elite::CMonster_Elite(const CMonster_Elite& Prototype)
	: CMonster_Melee{ Prototype }
{
}

HRESULT CMonster_Elite::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Elite.json";

	m_pPatterns = s_Patterns;
	m_iPatternDefCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	if (FAILED(Ready_AttackData()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Elite::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Elite");
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
	//CharDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	CharDesc.pRootBoneName = pDesc->pRootBoneName;
	CharDesc.bUseCCT = true;
	//추가 : 모델과 객체가 같은 챕터 레벨에 있다는 전제하에 진행 
	if (pArg != nullptr)
	{
		auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);
		CharDesc.iModelLevel = pDesc->iProtoLevel;
	}
	else CharDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_Weapon(*pDesc)))
		return E_FAIL;

	Set_IntroPattern(6);

	Spawn();

	return S_OK;
}

void CMonster_Elite::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
		Apply_Movment(fTimeDelta);
}

void CMonster_Elite::On_CheckDead()
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

void CMonster_Elite::Set_FatalMode(_bool bMode)
{
	const _bool bWasFatal = (nullptr != m_pBlackBoard) && m_pBlackBoard->Get<_bool>(L"bFatal");

	if (true == bMode && false == bWasFatal)
	{
		Request_AnimAction(L"FATAL");
		Set_AnimFloat(L"Speed", 0.f);

		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();

		m_bMoving = false;
		m_bMustAttack = false;

		Set_Groggy(false);
	}

	if (false == bMode)
	{
		if (true == Is_Dead())
		{
			m_pStatCom->Set_FatalDying(true);

			if (nullptr != m_pMovementCom && nullptr != m_pTransformCom)
			{
				_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
				_vector vAway = XMVectorNegate(m_pTransformCom->Get_State(STATE::LOOK));

				_vector vTargetPos;
				if (true == Get_TargetPos(vTargetPos))
					vAway = XMVectorSubtract(vMyPos, vTargetPos);

				m_pMovementCom->Begin_Launch(vAway, m_fLaunchSpeed, m_fLaunchUp);
			}
		}
	}

	m_pBlackBoard->Set<_bool>(L"bFatal", bMode);
}

void CMonster_Elite::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	Update_CombatParams(fTimeDelta);
	Update_Activation();

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
			Is_AnimInState(L"FATAL_DEAD_FRONT") || Is_AnimInState(L"FATAL_DEAD_BACK") ||
			Is_AnimInState(L"DEAD_BACK") || Is_AnimInState(L"DEAD_FRONT");
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
		{
			m_bSpawned = true;
		}
		else
		{
			if (false == Is_AnimInState(L"SPAWN"))
				Request_AnimAction(L"SPAWN");
			Mirror_ControllerParams();
			return;
		}
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
	Drive_Steering(fTimeDelta);
	Drive_MoveToTarget(fTimeDelta);
	Mirror_ControllerParams();
}

HRESULT CMonster_Elite::Ready_AttackData()
{
	m_AttackData[Hash_String(L"ATTACK_01")] = { 500.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_02")] = { 700.f, HIT_TYPE::Heavy };
	m_AttackData[Hash_String(L"ATTACK_03")] = { 600.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_04")] = { 800.f, HIT_TYPE::Heavy };
	m_AttackData[Hash_String(L"ATTACK_05")] = { 900.f, HIT_TYPE::Fury };
	m_AttackData[Hash_String(L"ATTACK_06")] = { 1000.f, HIT_TYPE::Fury };
	m_AttackData[Hash_String(L"ATTACK_07")] = { 700.f, HIT_TYPE::Heavy };

	return S_OK;
}

void CMonster_Elite::Update_PatternSelect(_float fTimeDelta)
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

	const _bool bCanCancel = (nullptr != m_pAnimator && m_pAnimator->Can_Cancel());
	const _bool bIdleReady = (true == bIdle && m_fIdleTimer >= m_fAttackGap);
	const _bool bCancelReady = (true == bCanCancel && false == m_bCancelConsumed);

	if (false == bCanCancel)
		m_bCancelConsumed = false;

	if (false == bIdleReady && false == bCancelReady)
		return;

	if (true == bCancelReady)
	{
		_vector vTargetPos;
		if (true == Get_TargetPos(vTargetPos))
		{
			m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
			if (false == Is_FacingTarget(cosf(XMConvertToRadians(15.f))))
				return;
		}
		m_bCancelConsumed = true;
	}

	if (true == Try_Begin_Intro())
		return;

	if (true == m_bMustAttack)
	{
		const _int iID = m_bLastMoveBack ? Select_LongPattern() : Select_WeightedPattern(m_fDistToTarget);
		Begin_Pattern(iID, Get_PatternLength(iID));
		m_bMustAttack = false;
		return;
	}

	const _int iCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));
	const _int iAct = Pick_CombatAction(m_fDistToTarget);
	if (iAct < iCount)
	{
		m_iLastPatternID = s_Patterns[iAct].iID;
		Begin_Pattern(s_Patterns[iAct].iID, s_Patterns[iAct].fAttackLength);
	}
	else
	{
		const _int iDir = rand() % 3;
		m_iMoveDir = (iDir == 0) ? static_cast<_int>(MON_DIRECTION::LEFT)
			: (iDir == 1) ? static_cast<_int>(MON_DIRECTION::RIGHT)
			: static_cast<_int>(MON_DIRECTION::BACK);
		m_bLastMoveBack = (iDir == 2);
		m_bMoving = true;
		m_fMoveTimer = m_fMoveDuration;
		m_fIdleTimer = 0.f;
	}
}


_int CMonster_Elite::Select_LongPattern()
{
	const _int iCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	_int iLong[16] = {};
	_int iNum = 0;
	for (_int i = 0; i < iCount; ++i)
		if (ATK_GROUP::LONG == s_Patterns[i].eGroup)
			iLong[iNum++] = s_Patterns[i].iID;

	if (0 == iNum)
		return Select_WeightedPattern(m_fDistToTarget);

	const _int iPick = iLong[rand() % iNum];
	m_iLastPatternID = iPick;
	return iPick;
}

void CMonster_Elite::Drive_MoveToTarget(_float fTimeDelta)
{
	if (nullptr == m_pAnimator || nullptr == m_pMovementCom)
		return;

	const _bool bMove = m_pAnimator->Is_InCustomWindow(L"MOVE_TO_TARGET");
	const _bool bRotate = m_pAnimator->Is_InCustomWindow(L"ROTATE_TO_TARGET");

	if (false == bMove && false == bRotate)
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	if (true == bMove)
	{
		m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
		m_pMovementCom->Set_MoveTarget(vTargetPos);
	}
	else if (true == bRotate)
	{
		m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
	}
}

const _wstring& CMonster_Elite::Get_KnockbackAction(_int iHitInfo) const
{
	static const _wstring strDamage = L"DAMAGE";
	return strDamage;
}

void CMonster_Elite::On_Hit_Post(const HitInfo& hitInfo)
{
	Wake_And_FaceHit(hitInfo);
}

_bool CMonster_Elite::Blocks_Damage_While_Groggy() const
{
	return true;
}

_float CMonster_Elite::Get_AttackRange() const
{
	return m_fFarAttackRange;
}

_bool CMonster_Elite::Wants_RunChase(_float fDist) const
{
	return fDist > m_fRunChaseRange;
}

CMonster_Elite* CMonster_Elite::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Elite* pInstance = new CMonster_Elite(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Elite");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Elite::Clone(void* pArg)
{
	CMonster_Elite* pInstance = new CMonster_Elite(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Elite");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Elite::Free()
{
	__super::Free();
}

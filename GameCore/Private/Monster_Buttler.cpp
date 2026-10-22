#include "Monster_Buttler.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"
#include "Model.h"
#include "Mesh.h"
#include "Weapon_Melee.h"
#include "Animator.h"

#include "Movement_AI.h"
#include "StatComponent_AI.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_Buttler, CMonster_Melee)
END_REFLECTION(CMonster_Buttler)
#endif

static const PATTERN_DEF s_Patterns[] =
{
	{ 1, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 2, ATK_GROUP::SHORT, 1.f, 1.5f },
	{ 3, ATK_GROUP::LONG,  0.5f, 7.f },
};

CMonster_Buttler::CMonster_Buttler(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Melee{ pDevice, pContext }
{
}

CMonster_Buttler::CMonster_Buttler(const CMonster_Buttler& Prototype)
	: CMonster_Melee{ Prototype }
{
}

HRESULT CMonster_Buttler::Initialize_Prototype()
{
	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Buttler.json";

	m_pPatterns = s_Patterns;
	m_iPatternDefCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	if (FAILED(Ready_AttackData()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Buttler::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Buttler");
		return E_FAIL;
	}

	MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);

	Apply_MeleeDesc(*pDesc);
	m_iPatternCount = pDesc->iPatternCount;
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

	CharDesc.fRunSpeed = pDesc->fSpeedPerSec;
	CharDesc.fSprintSpeed = pDesc->fSpeedPerSec * 1.5f;
	CharDesc.fCCTRadius = 0.8f;
	CharDesc.fCCTHeight = 1.2f;
	CharDesc.fCCTStepOffset = 0.1f;
	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_Weapon(*pDesc)))
		return E_FAIL;

	Spawn();

	return S_OK;
}

void CMonster_Buttler::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
		Apply_Movment(fTimeDelta);
}

void CMonster_Buttler::On_CheckDead()
{
	m_pStatCom->Set_FatalDying(false);
	m_bDead = true;

	Set_Groggy(false);
	m_pBlackBoard->Set<_bool>(L"bFatal", false);
	m_pBlackBoard->Set<_bool>(L"bHit", false);

	m_pStatCom->Reset_Groggy();
	m_pStatCom->Set_Staggerable(false);
	m_fHitTimer = 0.f;
}

void CMonster_Buttler::Set_FatalMode(_bool bFatal)
{
	const _bool bWasFatal = (nullptr != m_pBlackBoard) && m_pBlackBoard->Get<_bool>(L"bFatal");

	if (true == bFatal && false == bWasFatal)
	{
		Request_AnimAction(L"FATAL");
		Set_AnimFloat(L"Speed", 0.f);

		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();

		m_bMoving = false;
		m_bMustAttack = false;
	}

	if (false == bFatal)
	{
		if (false == Is_Dead())
			m_pStatCom->Set_CurrentHP(0.f);

		m_pStatCom->Set_FatalDying(true);
		Request_AnimAction(L"FATAL_DEAD");

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

	m_pBlackBoard->Set<_bool>(L"bFatal", bFatal);
}

void CMonster_Buttler::Apply_SpawnDesc(void* pArg)
{
	__super::Apply_SpawnDesc(pArg);

	if (nullptr == pArg)
		return;

	auto pDesc = static_cast<MONSTER_DESC*>(pArg);

	m_iSpawnNumber = (0 == pDesc->iSpawnNumber) ? 1 : pDesc->iSpawnNumber;
}

void CMonster_Buttler::Update_BlackBoard(_float fTimeDelta)
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
			Is_AnimInState(L"FALL_BACK_DEAD") || Is_AnimInState(L"FALL_FRONT_DEAD") ||
			Is_AnimInState(L"DEAD_BACK") || Is_AnimInState(L"DEAD_FRONT");

		if (false == bInDeadPose)
			Request_AnimAction(m_bDead ? L"FATAL_DEAD" : L"DEAD");

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
			const _bool bInSpawn =
				Is_AnimInState(L"SPAWN_01") || Is_AnimInState(L"SPAWN_02") || Is_AnimInState(L"SPAWN_03") ||
				Is_AnimInState(L"SPAWN_04") || Is_AnimInState(L"SPAWN_05");
			if (false == bInSpawn)
				Request_AnimAction(L"SPAWN");

			Mirror_ControllerParams();
			return;
		}
	}

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

HRESULT CMonster_Buttler::Ready_AttackData()
{
	m_AttackData[Hash_String(L"ATTACK_01")] = { 400.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_02")] = { 400.f, HIT_TYPE::Normal };
	m_AttackData[Hash_String(L"ATTACK_03")] = { 700.f, HIT_TYPE::Heavy };

	return S_OK;
}

void CMonster_Buttler::Update_PatternSelect(_float fTimeDelta)
{
	if (false == m_bSpawned)
		return;

	if (m_pBlackBoard->Get<_bool>(L"bHit"))
	{
		m_fHitTimer += fTimeDelta;
		const _bool bInReact =
			Is_AnimInState(L"DAMAGE_BACK") || Is_AnimInState(L"DAMAGE_LEFT") || Is_AnimInState(L"DAMAGE_RIGHT") ||
			Is_AnimInState(L"KNOCKBACK_BACK") || Is_AnimInState(L"KNOCKBACK_LEFT") || Is_AnimInState(L"KNOCKBACK_RIGHT");
		if (false == bInReact)
			Fire_Knockback();

		if (m_fHitTimer >= m_fHitMaxTime)
		{
			m_pBlackBoard->Set<_bool>(L"bHit", false);
			m_fHitTimer = 0.f;
		}

		m_bMoving = false;
		m_bMustAttack = false;
		m_iPendingPatternID = -1;
		m_bApproaching = false;
		return;
	}
	m_fHitTimer = 0.f;

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bDead"))
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

	if (true == m_bMustAttack)
	{
		const _int iID = m_bLastMoveBack ? 3 : Select_WeightedPattern(m_fDistToTarget);
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

void CMonster_Buttler::On_Hit_Pre(const HitInfo& hitInfo)
{
	Wake_And_FaceHit(hitInfo);
}

const _wstring& CMonster_Buttler::Get_KnockbackAction(_int iHitInfo) const
{
	static const _wstring strKnockback = L"KNOCKBACK";
	static const _wstring strDamage = L"DAMAGE";
	return iHitInfo > 0 ? strKnockback : strDamage;
}

void CMonster_Buttler::On_Mirror_Extra()
{
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
}

void CMonster_Buttler::On_Spawn_Post()
{
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
}

CMonster_Buttler* CMonster_Buttler::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Buttler* pInstance = new CMonster_Buttler(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Buttler");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Buttler::Clone(void* pArg)
{
	CMonster_Buttler* pInstance = new CMonster_Buttler(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Buttler");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Buttler::Free()
{
	__super::Free();
}

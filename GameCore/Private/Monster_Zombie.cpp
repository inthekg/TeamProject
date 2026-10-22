#include "Monster_Zombie.h"
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
BEGIN_REFLECTION(CMonster_Zombie, CMonster_Melee)
END_REFLECTION(CMonster_Zombie)
#endif

static const PATTERN_DEF s_Patterns[] =
{
	{ 1, ATK_GROUP::SHORT, 1.f, 2.0f },
	{ 2, ATK_GROUP::SHORT, 1.f, 2.0f },
	{ 3, ATK_GROUP::SHORT, 1.f, 2.0f },
};

CMonster_Zombie::CMonster_Zombie(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMonster_Melee{ pDevice, pContext }
{
}

CMonster_Zombie::CMonster_Zombie(const CMonster_Zombie& Prototype)
	: CMonster_Melee{ Prototype }
{
}

HRESULT CMonster_Zombie::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Elite.json";

	m_pPatterns = s_Patterns;
	m_iPatternDefCount = static_cast<_int>(sizeof(s_Patterns) / sizeof(PATTERN_DEF));

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Zombie::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Zombie");
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
	CharDesc.fGroundStick = -2.f;

	if (nullptr != pArg)
		CharDesc.iModelLevel = static_cast<AIOBJECT_DESC*>(pArg)->iProtoLevel;
	else
		CharDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_HitBoxes()))
		return E_FAIL;

	if (false == m_bEventSubscribed)
	{
		m_pGameInstance->Subscribe(EVENT_TYPE::BOSS_START, this);
		m_pGameInstance->Subscribe(EVENT_TYPE_MAP_OBJECT::CAGE_OPEN, this);
		m_bEventSubscribed = true;
	}

	Spawn();

	return S_OK;
}

void CMonster_Zombie::Apply_SpawnDesc(void* pArg)
{
	__super::Apply_SpawnDesc(pArg);

	if (nullptr == pArg)
		return;

	auto pDesc = static_cast<MONSTER_DESC*>(pArg);

	m_iSpawnNumber = pDesc->iSpawnNumber;
	m_iSpawnID = pDesc->iSpawnID;
}

void CMonster_Zombie::OnEvent(EVENT_TYPE eType, Event_Desc* pData)
{
	if (EVENT_TYPE::BOSS_START != eType)
		return;

	m_bBossStarted = true;

	if (true == Is_InActive())
		return;

	Set_AISleep(false);
}

void CMonster_Zombie::OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData)
{
	if (EVENT_TYPE_MAP_OBJECT::CAGE_OPEN != eType || nullptr == pData)
		return;

	CAGE_OPEN_EVENT_DESC* pEventDesc = static_cast<CAGE_OPEN_EVENT_DESC*>(pData);

	if (m_iSpawnID != pEventDesc->iTargetMonsterSpawnID || 2 != m_iSpawnNumber || SPAWN_PHASE::WAIT != m_eSpawnPhase)
		return;

	// Cage가 열려도 착지 전까지 SpawnNumber 2를 유지하고 CCT 낙하만 시작한다.
	Set_AnimBool(L"Falling", true);

	// Sector 수면 중이어도 Cage Open 직후부터 Update와 CCT 이동이 실행되게 한다.
	Set_AISleep(false);
	m_bSpawned = false;
	m_bAirborneOnce = false;
	m_fPhaseTimer = 0.f;
	m_iNaviCurCellIndex = -1;
	m_eSpawnPhase = SPAWN_PHASE::FALL;

	if (nullptr != m_pMovementCom)
	{
		m_pMovementCom->Stop_MoveTo();
		m_pMovementCom->Set_Grounded(false);
	}
}

void CMonster_Zombie::On_Spawn_Post()
{
	// Chapter를 넘어가도 풀 객체에 남아 다음 Chapter의 Cage 대기를 건너뛰던 값이다.
	m_bBossStarted = false;

	m_bAirborneOnce = false;
	m_fPhaseTimer = 0.f;
	m_iHomeCellIndex = -1;

	const _bool bWaitSpawn = (m_iSpawnNumber > 0) && (2 != m_iSpawnNumber || false == m_bBossStarted);

	m_eSpawnPhase = bWaitSpawn ? SPAWN_PHASE::WAIT : SPAWN_PHASE::DONE;
	m_bSpawned = (false == bWaitSpawn);

	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
	Set_AnimBool(L"Falling", false);

	// Cage 대기 Zombie는 Trigger 전까지 CCT 중력을 계산하지 않도록 Ground 상태로 고정한다.
	if (2 == m_iSpawnNumber && SPAWN_PHASE::WAIT == m_eSpawnPhase && nullptr != m_pMovementCom)
	{
		m_pMovementCom->Stop_MoveTo();
		m_pMovementCom->Set_Grounded(true);
	}
}

void CMonster_Zombie::On_Mirror_Extra()
{
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
}

void CMonster_Zombie::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	// SpawnNumber 2의 WAIT는 Cage Trigger 전용 상태이므로 위치와 중력을 완전히 고정한다.
	const _bool bHoldInCage = (2 == m_iSpawnNumber && SPAWN_PHASE::WAIT == m_eSpawnPhase);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal") && false == bHoldInCage)
		Apply_Movment(fTimeDelta);
}

void CMonster_Zombie::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
	{
		m_HitBoxes.On_StateChanged();
		m_HitBoxes.Set_HitType(HIT_TYPE::Normal);
	}

	m_HitBoxes.Update(m_pGameInstance, this, fTimeDelta);
}

void CMonster_Zombie::On_CheckDead()
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

void CMonster_Zombie::Set_FatalMode(_bool bMode)
{
	const _bool bWasFatal = (nullptr != m_pBlackBoard) && m_pBlackBoard->Get<_bool>(L"bFatal");

	if (true == bMode && false == bWasFatal)
	{
		Request_AnimAction(L"FATAL");
		Set_AnimFloat(L"Speed", 0.f);

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

void CMonster_Zombie::Update_BlackBoard(_float fTimeDelta)
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
			Is_AnimInState(L"DEAD_F") || Is_AnimInState(L"DEAD_B") ||
			Is_AnimInState(L"DEAD_HIT_F") || Is_AnimInState(L"DEAD_HIT_B");
		if (false == bInDeadPose)
			Request_AnimAction(L"DEAD");

		return;
	}

	if (SPAWN_PHASE::DONE != m_eSpawnPhase)
	{
		Drive_SpawnSequence(fTimeDelta);

		Set_AnimFloat(L"Speed", 0.f);
		m_pBlackBoard->Set<_bool>(L"bHit", false);
		m_fHitTimer = 0.f;
		m_bMoving = false;
		m_bMustAttack = false;
		m_iPendingPatternID = -1;
		m_bApproaching = false;

		Mirror_ControllerParams();
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
			Is_AnimInState(L"DAMAGE_BACK") || Is_AnimInState(L"DAMAGE_LEFT") || Is_AnimInState(L"DAMAGE_RIGHT");
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

void CMonster_Zombie::Drive_SpawnSequence(_float fTimeDelta)
{
	m_fPhaseTimer += fTimeDelta;

	switch (m_eSpawnPhase)
	{
	case SPAWN_PHASE::WAIT:
	{
		Set_AnimBool(L"Falling", false);

		if (false == Is_AnimInState(L"SPAWN_01") && false == Is_AnimInState(L"SPAWN_02"))
			Request_AnimAction(L"SPAWN");

		// SpawnNumber 2는 Player 감지나 Boss 시작으로 해제하지 않고 CAGE_OPEN 이벤트만 기다린다.
		if (2 == m_iSpawnNumber)
			break;

		const _bool bDetected =
			m_pBlackBoard->Get<_bool>(L"bPlayerInDetect") && m_pBlackBoard->Get<_bool>(L"bPlayerInFront");

		if (true == m_bBossStarted || true == bDetected)
			m_bSpawned = true;

		if (true == m_bSpawned)
		{
			m_bAirborneOnce = false;
			m_fPhaseTimer = 0.f;
			m_eSpawnPhase = (2 == m_iSpawnNumber) ? SPAWN_PHASE::FALL : SPAWN_PHASE::DONE;
		}
		break;
	}

	case SPAWN_PHASE::FALL:
	{
		m_iNaviCurCellIndex = -1;
		Set_AnimBool(L"Falling", true);

		const _bool bGrounded = (nullptr != m_pMovementCom) ? m_pMovementCom->Is_Grounded() : true;
		const _float fVelY = (nullptr != m_pMovementCom) ? m_pMovementCom->Get_FallVelocityY() : 0.f;

		if (false == m_bAirborneOnce)
		{
			// Cage Open 이벤트에서 Grounded를 false로 바꾸므로 짧은 낙하도 즉시 공중 상태로 확정한다.
			if (false == bGrounded || fVelY < m_fFallLatchVelY)
				m_bAirborneOnce = true;

			break;
		}

		if (true == bGrounded)
		{
			Capture_HomeCell();
			Enter_LandSpawn();
		}
		break;
	}

	case SPAWN_PHASE::LAND:
	{
		Set_AnimBool(L"Falling", false);

		Capture_HomeCell();

		// 착지 후 SPAWN_01 진입을 확인한 뒤에만 일반 전투 상태를 허용한다.
		if (false == Is_AnimInState(L"SPAWN_01") && m_fPhaseTimer < m_fLandTimeout)
		{
			Request_AnimState(L"SPAWN_01");
			break;
		}

		if (true == Is_AnimInState(L"SPAWN_01") || m_fPhaseTimer >= m_fLandTimeout)
		{
			m_eSpawnPhase = SPAWN_PHASE::DONE;
			m_bSpawned = true;
			m_bAggro = true;
			m_fLostTimer = 0.f;
		}
		break;
	}

	default:
		break;
	}
}

void CMonster_Zombie::Enter_LandSpawn()
{
	// 실제 착지 시점에 SpawnNumber 1로 전환하고 기상 상태를 요청한다.
	m_iSpawnNumber = 1;
	Set_AnimInt(L"SpawnNumber", m_iSpawnNumber);
	Set_AnimBool(L"Falling", false);
	Set_AnimBool(L"Spawn", false);
	Request_AnimSequenceEnd();
	Request_AnimState(L"SPAWN_01");

	m_eSpawnPhase = SPAWN_PHASE::LAND;
	m_fPhaseTimer = 0.f;
}

void CMonster_Zombie::Capture_HomeCell()
{
	if (m_iHomeCellIndex >= 0)
		return;

	if (nullptr == m_pNavigation || nullptr == m_pTransformCom)
		return;

	_float3 vPos = {};
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));

	if (false == m_pNavigation->Set_CurrentCell(vPos, &m_iNaviCurCellIndex))
		return;

	if (m_iNaviCurCellIndex < 0)
		return;

	m_iHomeCellIndex = m_iNaviCurCellIndex;

	_float3 vCenter = {};
	if (true == m_pNavigation->Get_CellCenter(static_cast<_uint>(m_iHomeCellIndex), &vCenter))
		m_vSpawnPos = _float4(vCenter.x, vCenter.y, vCenter.z, 1.f);
}

void CMonster_Zombie::Action_Notify(_uint64 iHashKey)
{
	__super::Action_Notify(iHashKey);

	auto itType = m_HitTypeKeys.find(iHashKey);
	if (itType != m_HitTypeKeys.end())
		m_HitBoxes.Set_HitType(itType->second);

	m_HitBoxes.Handle_Notify(iHashKey);
}

void CMonster_Zombie::Update_PatternSelect(_float fTimeDelta)
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

_int CMonster_Zombie::Select_LongPattern()
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

void CMonster_Zombie::Drive_MoveToTarget(_float fTimeDelta)
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

const _wstring& CMonster_Zombie::Get_KnockbackAction(_int iHitInfo) const
{
	static const _wstring strDamage = L"DAMAGE";
	return strDamage;
}

HRESULT CMonster_Zombie::Ready_HitBoxes()
{
	m_HitBoxes.Add_BoneSphere(L"HandL", "Bip001-L-Hand", 0.5f, 600.f);
	m_HitBoxes.Add_BoneSphere(L"HandR", "Bip001-R-Hand", 0.5f, 600.f);

	if (FAILED(m_HitBoxes.Build(m_pModelCom, m_pDevice, m_pContext)))
		return E_FAIL;

	return S_OK;
}

void CMonster_Zombie::On_Hit_Post(const HitInfo& hitInfo)
{
	Wake_And_FaceHit(hitInfo);
}

_bool CMonster_Zombie::Blocks_Damage_While_Groggy() const
{
	return true;
}

_float CMonster_Zombie::Get_AttackRange() const
{
	return m_fFarAttackRange;
}

_bool CMonster_Zombie::Wants_RunChase(_float fDist) const
{
	return fDist > m_fRunChaseRange;
}

CMonster_Zombie* CMonster_Zombie::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Zombie* pInstance = new CMonster_Zombie(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Zombie");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Zombie::Clone(void* pArg)
{
	CMonster_Zombie* pInstance = new CMonster_Zombie(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Zombie");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Zombie::Free()
{
	if (true == m_bEventSubscribed)
	{
		m_pGameInstance->UnSubscribe_AllEvent(this);
		m_bEventSubscribed = false;
	}

	m_HitBoxes.Free();

	__super::Free();
}

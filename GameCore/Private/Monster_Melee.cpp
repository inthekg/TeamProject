#include "Monster_Melee.h"
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
BEGIN_REFLECTION(CMonster_Melee, CAIObject)
END_REFLECTION(CMonster_Melee)
#endif

CMonster_Melee::CMonster_Melee(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
}

CMonster_Melee::CMonster_Melee(const CMonster_Melee& Prototype)
	: CAIObject{ Prototype }
	, m_AttackData{ Prototype.m_AttackData }
	, m_EffectKeys{ Prototype.m_EffectKeys }
	, m_pPatterns{ Prototype.m_pPatterns }
	, m_iPatternDefCount{ Prototype.m_iPatternDefCount }
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CMonster_Melee::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Melee::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	Set_EmissiveAmount(1.f);

	return S_OK;
}

void CMonster_Melee::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CMonster_Melee::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Melee::Late_Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
	{
		m_eCurHitType = HIT_TYPE::Normal;
		if(nullptr != m_pWeapon)
			m_pWeapon->Attack_End();
	}

	if (nullptr != m_pWeapon)
		m_pWeapon->Late_Update(fTimeDelta);
}

HRESULT CMonster_Melee::Render()
{
	return __super::Render();
}

void CMonster_Melee::On_Freeze(_bool bFreeze)
{
	if (true == bFreeze && nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
	}
}

void CMonster_Melee::On_Groggy(_bool bGroggy)
{
	if (true == bGroggy && nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
	}
	else
	{
		m_bFatalReady = false;
	}
}

void CMonster_Melee::On_SuperArmor(_bool bOn)
{
	if (bOn)
	{
		Start_LoopEffect(m_EffectKeys[EFFECT_KEY::FURY], "Effect_Monster_Fury", "Bip001-Spine1");
		Set_RimAmount(1.f);
		Set_RimProfile(1);
	}
	else
	{
		Stop_LoopEffect(m_EffectKeys[EFFECT_KEY::FURY]);
		Set_RimAmount(0.f);
	}
}

void CMonster_Melee::Action_Notify(_uint64 iHashKey)
{
	auto itType = m_HitTypeKeys.find(iHashKey);
	if (itType != m_HitTypeKeys.end())
	{
		m_eCurHitType = itType->second;
		return;
	}

	auto it = m_NotifyKeys.find(iHashKey);
	if (it == m_NotifyKeys.end())
		return;

	switch (static_cast<NOTIFY_KEY>(it->second))
	{
	case NOTIFY_KEY::WEAPON_ON:
	{
		if (nullptr == m_pWeapon)
			break;

		_float fDmg = m_fDamage;
		HIT_TYPE eType = m_eCurHitType;

		if (nullptr != m_pAnimator)
		{
			auto ita = m_AttackData.find(m_pAnimator->Get_CurControllerStateHash());
			if (ita != m_AttackData.end())
			{
				if (ita->second.fDamage >= 0.f)
					fDmg = ita->second.fDamage;
				if (ita->second.eHitType != HIT_TYPE::Normal)
					eType = ita->second.eHitType;
			}
		}

		m_pWeapon->Attack_Begin(fDmg, eType);
		break;
	}
	case NOTIFY_KEY::WEAPON_OFF:
		if (nullptr != m_pWeapon) { m_pWeapon->Attack_End(); }
		break;
	case NOTIFY_KEY::SET_DEAD:
		On_SetDead();
		break;
	case NOTIFY_KEY::CHECK_DEAD:
		On_CheckDead();
		break;
	case NOTIFY_KEY::FATAL_READY :
		m_bFatalReady = true;
		Begin_GroggyWindow(5.f);
		break;
	default:
		break;
	}
}

void CMonster_Melee::Spawn()
{
	__super::Spawn();

	m_bDead = false;
	m_bMoving = false;
	m_bMustAttack = false;
	m_bCancelConsumed = false;
	m_bLastMoveBack = false;

	m_iCurPatternID = 1;
	m_iLastPatternID = 0;
	m_iMoveDir = 0;

	m_bIntroDone = false;

	m_fIdleTimer = 0.f;
	m_fMoveTimer = 0.f;
	m_fDistToTarget = 9999.f;

	if (nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();
		m_pWeapon->Set_Render(true);
	}

	Stop_AllLoopEffects();

	if (CBT_BlackBoard* pBB = Get_BlackBoard())
	{
		pBB->Set<_bool>(L"bSpawned", false);
		pBB->Set<_bool>(L"bHit", false);
		pBB->Set<_bool>(L"bPlayerInDetect", false);
		pBB->Set<_bool>(L"bPlayerInFront", false);
		pBB->Set<_bool>(L"bInAttackRange", false);
		Set_Groggy(false);
	}

	On_Spawn_Post();
}

void CMonster_Melee::Despawn()
{
	__super::Despawn();

	Stop_AllLoopEffects();

	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();

}

void CMonster_Melee::On_SetDead()
{
	__super::On_SetDead();
	m_bDead = true;
	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();

	
}

_bool CMonster_Melee::Can_FatalFront() const
{
	if (m_pBlackBoard->Get<_bool>(L"bDead") || m_pBlackBoard->Get<_bool>(L"bFatal"))
		return false;

	if (m_pStatCom->Is_OnGroggyCooldown())
		return false;

	return m_bFatalReady;
}

_bool CMonster_Melee::Can_FatalBack() const
{
	if (m_pBlackBoard->Get<_bool>(L"bDead") || m_pBlackBoard->Get<_bool>(L"bFatal"))
		return false;

	if (m_pStatCom->Is_OnGroggyCooldown())
		return false;

	return true;
}

void CMonster_Melee::On_Groggy_Triggered()
{
	if (nullptr != m_pWeapon)
		m_pWeapon->Attack_End();

	m_bMoving = false;
	m_bMustAttack = false;
}

HRESULT CMonster_Melee::Ready_Weapon(const MELEE_DESC& Desc)
{
	if (Desc.strWeaponProtoTag.empty())
		return S_OK;

	const _float4x4* pSocket = Get_Bone_CombinedMatPtr(m_strWeaponSocketBone.c_str());
	if (nullptr == pSocket)
	{
		MSG_BOX("CMonster_Melee : weapon socket bone not found");
		return E_FAIL;
	}

	CWeapon_Melee::MELEE_DESC wd{};
	wd.pOwner = this;
	wd.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	wd.pSocketBoneMatrix = pSocket;
	wd.strModelTag = Desc.strWeaponProtoTag;
	wd.iLevelID = ETOUI(LEVEL::GAMEPLAY);
	wd.strTrailTopBoneName = m_strWeaponTrailTop;
	wd.strTrailBottomBoneName = m_strWeaponTrailBot;
	wd.strTrailFxTag = Desc.strWeaponTrailFx;
	wd.fSweepRadius = m_fSweepRadius;
	wd.fSweepHalfHeight = m_fSweepHalfHeight;
	wd.fDamage = m_fDamage;
	wd.bRender = true;

	if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon_Melee"), TEXT("Obj_Weapon"), &wd)))
		return E_FAIL;

	m_pWeapon = static_cast<CWeapon_Monster*>(Get_PartObject(TEXT("Obj_Weapon")));
	return S_OK;
}

HRESULT CMonster_Melee::Ready_NotifyKeys()
{
	m_NotifyKeys[Hash_String(L"HitOn_Weapon")] = NOTIFY_KEY::WEAPON_ON;
	m_NotifyKeys[Hash_String(L"HitOff_Weapon")] = NOTIFY_KEY::WEAPON_OFF;
	m_NotifyKeys[Hash_String(L"SET_DEAD")] = NOTIFY_KEY::SET_DEAD;
	m_NotifyKeys[Hash_String(L"CHECK_DEAD")] = NOTIFY_KEY::CHECK_DEAD;
	m_NotifyKeys[Hash_String(L"Symbol_Start")] = NOTIFY_KEY::FATAL_READY;

	m_EffectKeys[EFFECT_KEY::FURY] = Hash_String(L"LOOP_FURY");
	return S_OK;
}

void CMonster_Melee::Apply_MeleeDesc(const MELEE_DESC& Desc)
{
	m_fShortAttackRange = Desc.fShortAttackRange;
	m_fAttackGap = Desc.fAttackGap;
	m_fMoveDuration = Desc.fMoveDuration;
	m_fLoseDelay = Desc.fLoseDelay;
	m_fSweepRadius = Desc.fSweepRadius;
	m_fSweepHalfHeight = Desc.fSweepHalfHeight;

	if (false == Desc.strWeaponSocketBone.empty())
		m_strWeaponSocketBone = Desc.strWeaponSocketBone;
	if (false == Desc.strWeaponTrailTopBone.empty())
		m_strWeaponTrailTop = Desc.strWeaponTrailTopBone;
	if (false == Desc.strWeaponTrailBotBone.empty())
		m_strWeaponTrailBot = Desc.strWeaponTrailBotBone;

	Set_FatalOffset(_float3{ 0.f, 0.f, Desc.fFatalFront }, _float3{ 0.f, 0.f, -Desc.fFatalBack });
}

_float CMonster_Melee::Get_PatternLength(_int iID) const
{
	const _int iCount = m_iPatternDefCount;
	for (_int i = 0; i < iCount; ++i)
		if (m_pPatterns[i].iID == iID)
			return m_pPatterns[i].fAttackLength;
	return 0.f;
}

_int CMonster_Melee::Select_WeightedPattern(_float fDist)
{
	const _int iCount = m_iPatternDefCount;

	_float fW[16] = {};
	_float fTotal = 0.f;
	for (_int i = 0; i < iCount; ++i)
	{
		const _float fDiff = fabsf(fDist - m_pPatterns[i].fAttackLength);
		_float w = m_pPatterns[i].fWeight / (1.f + fDiff);
		if (m_pPatterns[i].iID == m_iLastPatternID)
			w *= 0.15f;
		fW[i] = w;
		fTotal += w;
	}
	if (fTotal <= 0.f)
		return 1;

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	_int iPick = 0;
	for (_int i = 0; i < iCount; ++i)
	{
		r -= fW[i];
		if (r <= 0.f) { iPick = i; break; }
	}
	m_iLastPatternID = m_pPatterns[iPick].iID;
	return m_pPatterns[iPick].iID;
}

_int CMonster_Melee::Pick_CombatAction(_float fDist)
{
	const _int iCount = m_iPatternDefCount;
	const _float W_MOVE = 2.f;

	_float fW[16] = {};
	_float fTotal = 0.f;
	for (_int i = 0; i < iCount; ++i)
	{
		const _float fDiff = fabsf(fDist - m_pPatterns[i].fAttackLength);
		_float w = m_pPatterns[i].fWeight / (1.f + fDiff);
		if (m_pPatterns[i].iID == m_iLastPatternID)
			w *= 0.15f;
		fW[i] = w;
		fTotal += w;
	}
	fW[iCount] = W_MOVE;
	fTotal += W_MOVE;

	_float r = (static_cast<_float>(rand()) / static_cast<_float>(RAND_MAX)) * fTotal;
	for (_int i = 0; i <= iCount; ++i)
	{
		r -= fW[i];
		if (r <= 0.f) return i;
	}
	return iCount;
}

void CMonster_Melee::Fire_Attack()
{
	Set_AnimInt(L"PatternID", m_iCurPatternID);
	Set_AnimTrigger(L"PatternStart");
	Request_AnimAction(L"ATTACK");
	m_fIdleTimer = 0.f;
}

void CMonster_Melee::Fire_Knockback()
{
	const _int iHitInfo = static_cast<_int>(m_pStatCom->Get_LastHitType());
	Set_AnimInt(L"Direction", m_iHitDir);
	Set_AnimInt(L"HitInfo", iHitInfo);
	Set_AnimTrigger(L"DamageTrigger");
	Request_AnimAction(Get_KnockbackAction(iHitInfo));
}

void CMonster_Melee::Update_CombatParams(_float fTimeDelta)
{
	const _bool bDead = m_pStatCom->Is_Dead();

	m_fDistToTarget = 9999.f;
	if (nullptr != m_pTransformCom && nullptr != m_pTarget)
	{
		if (CTransform* pTT = m_pTarget->Get_Transform())
		{
			_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vTo = XMVectorSubtract(
				XMVectorSetY(pTT->Get_State(STATE::POSITION), XMVectorGetY(vMyPos)), vMyPos);
			m_fDistToTarget = XMVectorGetX(XMVector3Length(vTo));
		}
	}

	Update_Aggro_WithLOS(m_fDistToTarget, fTimeDelta);

	if (true == bDead)
		m_bAggro = false;

	m_bDetected = (true == m_bSpawned && true == m_bAggro && false == m_bReturning);

	const _bool bInAttackRange = (m_fDistToTarget <= Get_AttackRange());
	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", m_bAggro);
	m_pBlackBoard->Set<_bool>(L"bInAttackRange", m_bAggro && bInAttackRange);
}

void CMonster_Melee::Mirror_ControllerParams()
{
	Set_AnimBool(L"Dead", (m_pBlackBoard->Get<_bool>(L"bDead") || m_bDead) && false == m_pStatCom->Is_FatalDying());
	Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
	Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
	Set_AnimBool(L"TargetCheck", m_pBlackBoard->Get<_bool>(L"bPlayerInDetect") && m_pBlackBoard->Get<_bool>(L"bPlayerInFront"));
	Set_AnimBool(L"Spawn", m_bSpawned);
	On_Mirror_Extra();
}

void CMonster_Melee::Drive_Steering(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") ||
		m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bDead"))
		return;

	if (false == m_bSpawned)
		return;

	if (false == m_bAggro)
		return;

	if (nullptr == m_pMovementCom)
		return;

	const _bool bInAttackRange = m_pBlackBoard->Get<_bool>(L"bInAttackRange");

	_vector vTargetPos;
	const _bool bHasTarget = Get_TargetPos(vTargetPos);

	if (false == bInAttackRange)
	{
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

	if (true == bHasTarget && true == Can_AnimRotate())
		m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);

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
		case MON_DIRECTION::BACK:  m_pMovementCom->Orbit(2, vTargetPos, fTimeDelta); break;
		default: break;
		}
	}
}

void CMonster_Melee::Drive_MoveToTarget(_float fTimeDelta)
{
	if (nullptr == m_pAnimator || nullptr == m_pMovementCom)
		return;

	if (false == m_pAnimator->Is_InCustomWindow(L"MOVE_TO_TARGET"))
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	m_pMovementCom->Set_SpeedType(SPEED_TYPE::SPRINT);
	m_pMovementCom->Set_MoveTarget(vTargetPos);
}

_bool CMonster_Melee::Is_FacingTarget(_float fCosThreshold) const
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
	return XMVectorGetX(XMVector3Dot(vLook, vTo)) >= fCosThreshold;
}

_bool CMonster_Melee::Is_AnimIdle() const
{
	return Is_AnimInState(L"IDLE");
}

void CMonster_Melee::Wake_And_FaceHit(const HitInfo& hitInfo)
{
	Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));
	Set_AnimInt(L"Direction", m_iHitDir);
	m_pBlackBoard->Set<_bool>(L"bSpawned", true);
	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", true);
}

_bool CMonster_Melee::Try_Begin_Intro()
{
	if (true == m_bIntroDone || m_iIntroPatternID < 0)
		return false;

	m_bIntroDone = true;

	_float fLen = Get_PatternLength(m_iIntroPatternID);
	if (fLen <= 0.f)
		fLen = m_fShortAttackRange;

	Begin_Pattern(m_iIntroPatternID, fLen);
	return true;
}

const _wstring& CMonster_Melee::Get_KnockbackAction(_int iHitInfo) const
{
	static const _wstring strKnockback = L"KNOCKBACK";
	static const _wstring strDamage = L"DAMAGE";
	return iHitInfo > 0 ? strKnockback : strDamage;
}

#ifdef _DEBUG
_int CMonster_Melee::Get_DebugMoveState() const
{
	const _int iBase = __super::Get_DebugMoveState();

	if (iBase == static_cast<_int>(DBG_MOVE::COMBAT) && true == m_bMoving)
		return static_cast<_int>(DBG_MOVE::STRAFE);

	return iBase;
}
#endif

void CMonster_Melee::Free()
{
	__super::Free();
}

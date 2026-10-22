#include "Monster_Range.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"
#include "CCT.h"
#include "Model.h"
#include "Weapon_Range.h"
#include "Animator.h"

#include "StatComponent_AI.h"
#include "Movement_AI.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_Range, CAIObject)
END_REFLECTION(CMonster_Range)
#endif

CMonster_Range::CMonster_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
}

CMonster_Range::CMonster_Range(const CMonster_Range& Prototype)
	: CAIObject{ Prototype }
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CMonster_Range::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Range.json";

	if (FAILED(Ready_NotifyKeys()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMonster_Range::Initialize(void* pArg)
{	
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CMonster_Range");
		return E_FAIL;
	}

	MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);
	m_fAttackRange = pDesc->fAttackRange;
	m_fDetectRange = pDesc->fAttackRange;
	m_fChaseRange = pDesc->fChaseRange;
	m_fLeashRange = pDesc->fLeashRange;
	m_fAttackGap = pDesc->fAttackGap;
	m_fLoseDelay = pDesc->fLoseDelay;
	m_fBulletDamage = pDesc->fBulletDamage;
	m_fBulletSpeed = pDesc->fBulletSpeed;
	m_fBulletRange = 120.f;
	m_fBulletRadius = pDesc->fBulletRadius;
	m_strWeaponSocketBone = pDesc->strWeaponSocketBone;
	m_strFireBoneName = pDesc->strFireBoneName;

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	Set_FatalOffset(_float3{ 0.f, 0.f, pDesc->fFatalFront }, _float3{ 0.f, 0.f, -pDesc->fFatalBack });
	Set_EmissiveAmount(1.f);
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

	Spawn();

	return S_OK;
}

void CMonster_Range::Spawn()
{
	__super::Spawn();

	m_bDead = false;
	m_fIdleTimer = 0.f;
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

	m_bHasLOS = false;
}

void CMonster_Range::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	Capture_SpawnPos();

	__super::Update(fTimeDelta);

	if (false == m_pBlackBoard->Get<_bool>(L"bFatal"))
		Apply_Movment(fTimeDelta);
}

void CMonster_Range::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Range::Late_Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	if (true == Was_AnimStateChanged())
		m_eCurHitType = HIT_TYPE::Normal;

	if (nullptr != m_pWeapon)
		m_pWeapon->Late_Update(fTimeDelta);
}

HRESULT CMonster_Range::Render()
{
	return __super::Render();
}

void CMonster_Range::On_Freeze(_bool bFreeze)
{
}

void CMonster_Range::Action_Notify(_uint64 iHashKey)
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
	case NOTIFY_KEY::FIRE:
		Fire_Shot();
		break;
	case NOTIFY_KEY::SET_DEAD:
		On_SetDead();
		break;
	case NOTIFY_KEY::CHECK_DEAD:
		On_CheckDead();
		break;
	case NOTIFY_KEY::FATAL_READY:
		m_bFatalReady = true;
		Begin_GroggyWindow(5.f);
		break;
	default:
		break;
	}
}

void CMonster_Range::On_CheckDead()
{
	m_bDead = true;

	m_pBlackBoard->Set<_bool>(L"bFatal", false);
	m_pBlackBoard->Set<_bool>(L"bHit", false);

	m_pStatCom->Reset_Groggy();
	m_pStatCom->Set_Staggerable(false);
	m_fHitTimer = 0.f;	
}

void CMonster_Range::On_SetDead()
{
	__super::On_SetDead();
	m_bDead = true;
	m_pBlackBoard->Set<_bool>(L"bFatal", false);
	m_pBlackBoard->Set<_bool>(L"bHit", false);

	m_pStatCom->Reset_Groggy();
	m_pStatCom->Set_Staggerable(false);
	m_fHitTimer = 0.f;
	//Request_Despawn();
}

_bool CMonster_Range::Can_FatalFront() const
{
	if (m_pBlackBoard->Get<_bool>(L"bDead") || m_pBlackBoard->Get<_bool>(L"bFatal"))
		return false;

	if (m_pStatCom->Is_OnGroggyCooldown())
		return false;

	return m_bFatalReady;
}

_bool CMonster_Range::Can_FatalBack() const
{
	if (m_pBlackBoard->Get<_bool>(L"bDead") || m_pBlackBoard->Get<_bool>(L"bFatal"))
		return false;

	if (m_pStatCom->Is_OnGroggyCooldown())
		return false;

	return true;
}

void CMonster_Range::On_Groggy(_bool bGroggy)
{
	if (bGroggy == false)
		m_bFatalReady = false;
}

void CMonster_Range::Despawn()
{
	__super::Despawn();

	Stop_AllLoopEffects();

	if (nullptr != m_pWeapon)
	{
		m_pWeapon->Attack_End();		
	}
}

void CMonster_Range::Set_FatalMode(_bool bMode)
{
	const _bool bWasFatal = (nullptr != m_pBlackBoard) && m_pBlackBoard->Get<_bool>(L"bFatal");

	if (true == bMode && false == bWasFatal)
	{
		Request_AnimAction(L"FATAL");
		Set_AnimFloat(L"Speed", 0.f);

		if (nullptr != m_pWeapon)
			m_pWeapon->Attack_End();

		Set_Groggy(false);
	}

	if (false == bMode)
	{
		if (false == Is_Dead())
			m_pStatCom->Set_CurrentHP(0.f);

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

	m_pBlackBoard->Set<_bool>(L"bFatal", bMode);
}

void CMonster_Range::Update_BlackBoard(_float fTimeDelta)
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
		Cancel_Leash();
		Mirror_ControllerParams();
		return;
	}
	m_fHitTimer = 0.f;

	if (m_fDistToTarget <= m_fAttackRange)
		Cancel_Leash();

	Update_ShootSelect(fTimeDelta);
	Drive_Steering(fTimeDelta);
	Drive_Aim(fTimeDelta);
	Mirror_ControllerParams();
}


void CMonster_Range::Update_CombatParams(_float fTimeDelta)
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

	const _bool bInAttack = (m_fDistToTarget <= m_fAttackRange);

	m_bHasLOS = false;
	if (true == m_bAggro)
	{
		_vector vFrom, vTargetPos;
		if (true == Get_FireOrigin(vFrom) && true == Get_TargetPos(vTargetPos))
		{
			vTargetPos = XMVectorAdd(vTargetPos, XMVectorSet(0.f, 1.2f, 0.f, 0.f));
			m_bHasLOS = Has_LineOfSight(vFrom, vTargetPos, m_ePerceiveBlockers, m_fPerceiveLOSRadius);
		}
	}

	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", m_bAggro);
	m_pBlackBoard->Set<_bool>(L"bInAttackRange", m_bAggro && bInAttack && m_bHasLOS);
}


void CMonster_Range::Update_ShootSelect(_float fTimeDelta)
{
	if (false == m_bSpawned)
		return;

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") || m_pBlackBoard->Get<_bool>(L"bDead"))
		return;

	if (false == m_pBlackBoard->Get<_bool>(L"bInAttackRange"))
		return;

	const _bool bIdle = Is_AnimIdle();
	m_fIdleTimer = bIdle ? (m_fIdleTimer + fTimeDelta) : 0.f;

	if (false == bIdle || m_fIdleTimer < m_fAttackGap)
		return;

	if (true == Is_FacingTarget(cosf(XMConvertToRadians(20.f))))
	{
 		Set_AnimInt(L"PatternID", 1);
		Set_AnimTrigger(L"PatternStart");
		Request_AnimAction(L"ATTACK");
		m_fIdleTimer = 0.f;
	}
}

void CMonster_Range::Fire_Shot()
{
	if (nullptr == m_pWeapon)
		return;

	_vector vOrigin;
	if (false == Get_FireOrigin(vOrigin))
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	vTargetPos = XMVectorAdd(vTargetPos, XMVectorSet(0.f, 1.2f, 0.f, 0.f));

	if (false == Has_LineOfSight(vOrigin, vTargetPos, m_ePerceiveBlockers, m_fPerceiveLOSRadius))
		return;

	_vector vDir = XMVector3Normalize(XMVectorSubtract(vTargetPos, vOrigin));

	if (nullptr != m_pTransformCom)
	{
		_vector vLook = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
		_vector vDirFlat = XMVectorSetY(vDir, 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vDirFlat)) < 1e-6f)
			vDirFlat = vLook;
		else
			vDirFlat = XMVector3Normalize(vDirFlat);

		const _float fCos = XMVectorGetX(XMVector3Dot(vLook, vDirFlat));
		if (fCos < cosf(XMConvertToRadians(60.f)))
			vDir = vLook;
	}

	m_pWeapon->Fire(vOrigin, vDir, m_fBulletDamage, m_eCurHitType);
}


void CMonster_Range::Fire_Knockback()
{
	const _int iHitInfo = static_cast<_int>(m_pStatCom->Get_LastHitType());
	Set_AnimInt(L"Direction", m_iHitDir);
	Set_AnimInt(L"HitInfo", iHitInfo);
	Set_AnimTrigger(L"DamageTrigger");
	Request_AnimAction(L"DAMAGE");
}

void CMonster_Range::Mirror_ControllerParams()
{
	Set_AnimBool(L"Dead", (m_pBlackBoard->Get<_bool>(L"bDead") || m_bDead) && false == m_pStatCom->Is_FatalDying());
	Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
	Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
	Set_AnimBool(L"TargetCheck", m_pBlackBoard->Get<_bool>(L"bPlayerInDetect") && m_pBlackBoard->Get<_bool>(L"bPlayerInFront"));
	Set_AnimBool(L"Spawn", m_bSpawned);
}

void CMonster_Range::Drive_Steering(_float fTimeDelta)
{
	Set_AnimFloat(L"Speed", 0.f);

	if (m_pBlackBoard->Get<_bool>(L"bGroggy") || m_pBlackBoard->Get<_bool>(L"bFatal") ||
		m_pBlackBoard->Get<_bool>(L"bHit") || m_pBlackBoard->Get<_bool>(L"bDead"))
		return;

	if (false == m_bSpawned)
		return;

	if (false == m_bAggro)
		return;

	_vector vTargetPos;
	if (true == Get_TargetPos(vTargetPos) && true == Can_AnimRotate())
		m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
}

void CMonster_Range::Drive_Aim(_float fTimeDelta)
{
	if (nullptr == m_pAnimator)
		return;

	if (false == m_pAnimator->Is_InCustomWindow(L"ROTATE_TO_TARGET"))
		return;

	_vector vTargetPos;
	if (false == Get_TargetPos(vTargetPos))
		return;

	m_pMovementCom->TurnTo(vTargetPos, fTimeDelta);
}

_bool CMonster_Range::Get_FireOrigin(_vector& vOut)
{
	const _float4x4* pBone = Get_Bone_CombinedMatPtr(m_strFireBoneName.c_str());
	if (nullptr == pBone || nullptr == m_pTransformCom)
		return false;

	_matrix matBone = XMLoadFloat4x4(pBone);
	_matrix matWorld = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	_matrix matFire = XMMatrixMultiply(matBone, matWorld);
	vOut = matFire.r[3];
	return true;
}

_bool CMonster_Range::Is_FacingTarget(_float fCosThreshold) const
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

_bool CMonster_Range::Is_AnimIdle() const
{
	return Is_AnimInState(L"IDLE");
}

void CMonster_Range::On_Hit_Post(const HitInfo& hitInfo)
{
	Compute_HitDir(XMLoadFloat3(&hitInfo.vHitPosition));
	Set_AnimInt(L"Direction", m_iHitDir);
	m_pBlackBoard->Set<_bool>(L"bSpawned", true);
	m_pBlackBoard->Set<_bool>(L"bPlayerInDetect", true);
}

_bool CMonster_Range::Blocks_Damage_While_Groggy() const
{
	return true;
}

void CMonster_Range::Apply_ChargeHit(_float fDamage, _float fGroggy)
{
	m_pStatCom->Apply_Damage(fDamage);
	if (Is_Dead())
	{
		m_pBlackBoard->Set<_bool>(L"bDead", true);
		return;
	}

	m_pStatCom->Set_SinceHit(0.f);

	if (true == m_pStatCom->Is_Staggerable() && false == m_pStatCom->Is_OnGroggyCooldown())
	{
		m_pStatCom->Reset_Groggy();
		m_pStatCom->Set_Staggerable(false);
		Set_Groggy(true);

		Request_AnimAction(L"GROGGY");
		Set_AnimFloat(L"Speed", 0.f);
	}
	else if (false == Is_Groggy())
	{
		if (false == Is_SuperArmor())
			m_pBlackBoard->Set<_bool>(L"bHit", true);

		m_pStatCom->Add_Groggy(fGroggy);
		if (m_pStatCom->Is_GroggyFull())
			m_pStatCom->Set_Staggerable(true);
	}
}


HRESULT CMonster_Range::Ready_NotifyKeys()
{
	m_NotifyKeys[Hash_String(L"FIRE_ONCE")]		= NOTIFY_KEY::FIRE;
	m_NotifyKeys[Hash_String(L"SET_DEAD")]		= NOTIFY_KEY::SET_DEAD;
	m_NotifyKeys[Hash_String(L"CHECK_DEAD")]	= NOTIFY_KEY::CHECK_DEAD;
	m_NotifyKeys[Hash_String(L"Symbol_Start")] = NOTIFY_KEY::FATAL_READY;
	return S_OK;
}

HRESULT CMonster_Range::Ready_Weapon(const MONSTER_DESC& Desc)
{
	if (Desc.strWeaponProtoTag.empty())
		return S_OK;

	const _float4x4* pSocket = Get_Bone_CombinedMatPtr(m_strWeaponSocketBone.c_str());
	if (nullptr == pSocket)
	{
		MSG_BOX("CMonster_Range : weapon socket bone not found");
		return E_FAIL;
	}

	CWeapon_Range::RANGE_DESC wd{};
	wd.pOwner = this;
	wd.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	wd.pSocketBoneMatrix = pSocket;
	wd.strModelTag = Desc.strWeaponProtoTag;
	wd.iLevelID = ETOUI(LEVEL::GAMEPLAY);
	wd.fDamage = m_fBulletDamage;
	wd.bRender = true;

	wd.fRange =		m_fBulletRange;
	wd.fBulletSpeed = m_fBulletSpeed;
	wd.fBulletRadius = m_fBulletRadius;
	wd.eFireMode = CWeapon_Range::FIRE_MODE::PROJECTILE;

	if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon_Range"), TEXT("Obj_Weapon"), &wd)))
		return E_FAIL;

	m_pWeapon = static_cast<CWeapon_Range*>(Get_PartObject(TEXT("Obj_Weapon")));
	return S_OK;
}

CMonster_Range* CMonster_Range::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Range* pInstance = new CMonster_Range(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Range");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CMonster_Range::Clone(void* pArg)
{
	CMonster_Range* pInstance = new CMonster_Range(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Range");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CMonster_Range::Free()
{
	__super::Free();	
}

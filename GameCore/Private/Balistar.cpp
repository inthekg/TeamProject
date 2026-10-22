#include "Balistar.h"
#include "GameInstance.h"
#include "Transform.h"
#include "Model.h"
#include "Bone.h"
#include "Animator.h"
#include "Character.h"
#include "StatComponent_AI.h"
#include "Shooter.h"

#include "Monster_Include.h"


#ifdef _DEBUG
BEGIN_REFLECTION(CBalistar, CAIObject)
END_REFLECTION(CBalistar)
#endif

namespace
{
	static const _float		TARGET_AIM_HEIGHT = 1.0f;
}

CBalistar::CBalistar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
}

CBalistar::CBalistar(const CBalistar& Prototype)
	: CAIObject{ Prototype }
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBalistar::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Balistar_BT.json";

	return S_OK;
}

HRESULT CBalistar::Initialize(void* pArg)
{
	if (nullptr == pArg)
	{
		MSG_BOX("Null Argument : CBalistar");
		return E_FAIL;
	}

	BALISTAR_DESC* pDesc = static_cast<BALISTAR_DESC*>(pArg);

	m_strFireBone = pDesc->strFireBone;
	m_strGazeBaseBone = pDesc->strGazeBaseBone;
	m_strRootBone = pDesc->strRootBoneName;
	m_fBulletSpeed = pDesc->fBulletSpeed;
	m_fBulletRange = pDesc->fBulletRange;
	m_fBulletRadius = pDesc->fBulletRadius;
	m_fBulletDamage = pDesc->fBulletDamage;
	m_iMaxHit = pDesc->iMaxHit;
	m_fLoseGrace = pDesc->fLoseGrace;

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	Set_EmissiveAmount(1.f);

	CHARACTER_SETUP_DESC CharDesc{};
	CharDesc.strModelPrototypeTag = pDesc->strModelProtoTag;
	CharDesc.strAnimFolderTag = pDesc->strAnimFolderTag;
	CharDesc.strAnimTag = pDesc->strAnimFolderTag;
	CharDesc.pRootBoneName = m_strRootBone.c_str();
	CharDesc.iModelLevel = pDesc->iProtoLevel;
	CharDesc.bUseCCT = true;
	CharDesc.fCCTRadius = 0.8f;
	CharDesc.fCCTHeight = 1.6f;

	if (FAILED(Ready_Character(CharDesc)))
		return E_FAIL;

	if (FAILED(Ready_Shooter()))
		return E_FAIL;

	Spawn();

	return S_OK;
}

void CBalistar::Spawn()
{
	__super::Spawn();

	m_ePhase = PHASE::IDLE;
	m_iHitCount = 0;
	m_bDeathStarted = false;
	m_bAimInit = false;
	m_bPitchInit = false;
	m_bAggro = false;
	m_fAimTime = 0.f;
	m_fLoseTimer = 0.f;
	m_fPhaseTimer = 0.f;
	m_fDebugTimer = 0.f;

	Set_AnimBool(L"Detected", false);
	Set_AnimBool(L"Returning", false);
	Set_AnimBool(L"Dead", false);
}

void CBalistar::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	if (nullptr != m_pShooter)
		m_pShooter->Update(fTimeDelta);
}

void CBalistar::Late_Update(_float fTimeDelta)
{
	if (false == Is_Dead() && false == Is_InActive() && nullptr != m_pModelCom && nullptr != m_pAnimator)
	{
		switch (m_ePhase)
		{
		case PHASE::AIM:
		case PHASE::RELOAD:
			Drive_Aim(fTimeDelta, true, false);
			break;
		case PHASE::ATTACK:
			Drive_Aim(fTimeDelta, true, true);
			break;
		case PHASE::RETURN:
			Drive_Aim(fTimeDelta, false, false);
			break;
		default:
			break;
		}
	}

	__super::Late_Update(fTimeDelta);

	if (nullptr != m_pShooter)
		m_pShooter->Late_Update(fTimeDelta);
}

void CBalistar::Drive_Aim(_float fTimeDelta, _bool bToPlayer, _bool bSnap)
{
	POSE Pose = m_pAnimator->Get_CurPose();
	m_pModelCom->Update_Bones(&Pose);

	const _float fSlerpAmt = min(1.f, m_fAimSlerpRate * fTimeDelta);

	_float fYaw = 0.f;
	if (true == bToPlayer && true == Get_AimArc(fYaw))
	{
		if (fYaw > m_fGazeHalfRad)
			fYaw = m_fGazeHalfRad;
		else if (fYaw < -m_fGazeHalfRad)
			fYaw = -m_fGazeHalfRad;
	}
	else
		fYaw = 0.f;
	Apply_BoneRot(Pose, m_strAimBone, m_vAimRotCur, m_bAimInit, fYaw, bSnap, fSlerpAmt);

	m_pModelCom->Update_Bones(&Pose);

	_float fPitch = 0.f;
	if (true == bToPlayer && true == Get_AimPitch(fPitch))
	{
		if (fPitch > m_fPitchHalfRad)
			fPitch = m_fPitchHalfRad;
		else if (fPitch < -m_fPitchHalfRad)
			fPitch = -m_fPitchHalfRad;
	}
	else
		fPitch = 0.f;
	Apply_BoneRot(Pose, m_strPitchBone, m_vPitchRotCur, m_bPitchInit, fPitch, bSnap, fSlerpAmt);

	m_pModelCom->Update_Bones(&Pose);
	m_bSkipBoneUpdateOnce = true;
}

void CBalistar::Apply_BoneRot(POSE& Pose, const string& strBone, _float4& refCur, _bool& refInit, _float fArc, _bool bSnap, _float fSlerpAmt)
{
	const _int iIdx = m_pModelCom->Get_BoneIndex(strBone.c_str());
	if (iIdx < 0)
		return;

	const POSE& BindPose = *m_pModelCom->Get_BindPosePtr();
	const _vector qBind = XMLoadFloat4(&BindPose[iIdx].vRot);
	const _vector qDelta = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), fArc);
	_vector qTarget = XMQuaternionMultiply(qDelta, qBind);

	if (false == refInit)
	{
		refCur = Pose[iIdx].vRot;
		refInit = true;
	}

	_vector qCur = XMLoadFloat4(&refCur);
	if (XMVectorGetX(XMQuaternionDot(qCur, qTarget)) < 0.f)
		qTarget = XMVectorNegate(qTarget);

	if (true == bSnap)
	{
		qCur = qTarget;
	}
	else
	{
		const _float fDot = XMVectorGetX(XMQuaternionDot(qCur, qTarget));
		if (fDot < cosf(m_fAimErrorRad * 0.5f))
			qCur = XMQuaternionSlerp(qCur, qTarget, fSlerpAmt);
		else
			qCur = qTarget;
	}
	XMStoreFloat4(&refCur, qCur);

	XMStoreFloat4(&Pose[iIdx].vRot, qCur);
}

_bool CBalistar::Get_AimPitch(_float& OutPitch)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return false;

	const _float4x4* pDrum = m_pModelCom->Get_Bone_CombinedMatPtr(m_strPitchBone.c_str());
	if (nullptr == pDrum)
		return false;

	_vector vAim;
	if (false == Get_TargetAimPos(vAim))
		return false;

	const _matrix matDrumW = XMLoadFloat4x4(pDrum) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	_vector vDelta = XMVectorSubtract(vAim, matDrumW.r[3]);

	const _float fUp = XMVectorGetY(vDelta);
	_vector vFlat = XMVectorSetY(vDelta, 0.f);
	const _float fHorz = XMVectorGetX(XMVector3Length(vFlat));
	if (fHorz < 1e-4f)
		return false;

	OutPitch = m_fPitchScale * atan2f(fUp, fHorz) + m_fPitchOffset;
	return true;
}

_bool CBalistar::Get_AimArc(_float& OutArc)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return false;

	const _float4x4* pParent = m_pModelCom->Get_Bone_CombinedMatPtr(m_strAimParentBone.c_str());
	if (nullptr == pParent)
		return false;

	_vector vAim;
	if (false == Get_TargetAimPos(vAim))
		return false;

	const _matrix matParentW = XMLoadFloat4x4(pParent) * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());

	_vector vAimW = XMVectorSubtract(vAim, matParentW.r[3]);
	if (XMVectorGetX(XMVector3LengthSq(vAimW)) < 1e-6f)
		return false;
	vAimW = XMVector3Normalize(vAimW);

	const _matrix matParentInv = XMMatrixInverse(nullptr, matParentW);
	_vector vAimLocal = XMVector3Normalize(XMVector3TransformNormal(vAimW, matParentInv));
	const _float fLy = XMVectorGetY(vAimLocal);
	const _float fLz = XMVectorGetZ(vAimLocal);
	if (fLy * fLy + fLz * fLz < 1e-6f)
		return false;

	_float fAngle = m_fAimYawScale * atan2f(fLy, fLz) + m_fAimYawOffset;
	while (fAngle > XM_PI)
		fAngle -= XM_2PI;
	while (fAngle < -XM_PI)
		fAngle += XM_2PI;

	OutArc = fAngle;
	return true;
}

_bool CBalistar::Is_AimNeutral() const
{
	if (nullptr == m_pModelCom)
		return true;

	const _float fCos = cosf(m_fAimErrorRad * 0.5f);
	const POSE& BindPose = *m_pModelCom->Get_BindPosePtr();

	const _int iYaw = m_pModelCom->Get_BoneIndex(m_strAimBone.c_str());
	if (iYaw >= 0)
	{
		const _vector qCur = XMLoadFloat4(&m_vAimRotCur);
		const _vector qBind = XMLoadFloat4(&BindPose[iYaw].vRot);
		if (fabsf(XMVectorGetX(XMQuaternionDot(qCur, qBind))) < fCos)
			return false;
	}

	const _int iPitch = m_pModelCom->Get_BoneIndex(m_strPitchBone.c_str());
	if (iPitch >= 0)
	{
		const _vector qCur = XMLoadFloat4(&m_vPitchRotCur);
		const _vector qBind = XMLoadFloat4(&BindPose[iPitch].vRot);
		if (fabsf(XMVectorGetX(XMQuaternionDot(qCur, qBind))) < fCos)
			return false;
	}

	return true;
}

void CBalistar::Update_BlackBoard(_float fTimeDelta)
{
	if (true == Is_Dead())
	{
		Drive_Death();
		return;
	}

	if (nullptr == m_pTarget || nullptr == m_pAnimator)
		return;

	const _bool bPerceive = Perceive_Now();

	switch (m_ePhase)
	{
	case PHASE::IDLE:
		if (true == bPerceive)
			Enter_Aim(true);
		break;

	case PHASE::AIM:
		if (false == bPerceive)
		{
			m_fLoseTimer += fTimeDelta;
			if (m_fLoseTimer >= m_fLoseGrace)
				Enter_Return();
		}
		else
		{
			m_fLoseTimer = 0.f;
			m_fAimTime += fTimeDelta;
			if (m_fAimTime >= m_fAimDuration)
				Begin_Attack();
		}
		break;

	case PHASE::ATTACK:
	case PHASE::RELOAD:
		m_fPhaseTimer += fTimeDelta;
		if (m_fPhaseTimer >= m_fPhaseTimeout)
		{
			if (true == bPerceive)
				Enter_Aim(false);
			else
				Enter_Return();
		}
		break;

	case PHASE::RETURN:
		if (true == bPerceive)
			Enter_Aim(false);
		else if (true == Is_AimNeutral())
			Enter_Idle();
		break;
	}
}

void CBalistar::Action_Notify(_uint64 iHashKey)
{
	if (iHashKey == Hash_String(L"FIRE_ONCE"))
	{
		if (false == Is_Dead())
			Fire_Projectile();
		m_pGameInstance->Play_Effect("Effect_Balista_Muzzle", m_pModelCom->Get_Bone_CombinedMatPtr("SD_SK_Muzzle"), m_pTransformCom->Get_WorldMatrixPtr());
	}
	else if (iHashKey == Hash_String(L"RELOAD_START"))
	{
		m_ePhase = PHASE::RELOAD;
		m_fPhaseTimer = 0.f;
		Request_AnimAction(L"RELOAD");
	}
	else if (iHashKey == Hash_String(L"IDLE_START"))
	{
		if (true == Perceive_Now())
			Enter_Aim(false);
		else
			Enter_Return();
	}
	else if (iHashKey == Hash_String(L"SET_DEAD"))
	{
		On_SetDead();
	}
}

_bool CBalistar::Take_Damage(const HitInfo& hitInfo)
{
	if (true == Is_Dead() || true == Is_InActive())
		return true;

	m_fHudHitTimer = m_fHudHitShowTime;

	++m_iHitCount;

	if (m_iHitCount >= m_iMaxHit)
	{
		if (nullptr != m_pStatCom)
			m_pStatCom->Set_CurrentHP(0.f);
	}
	else
	{
		Request_AnimAction(L"DAMAGED");

		if (true == Perceive_Now())
			Enter_Aim(false);
		else
			Enter_Return();
	}

	return true;
}

void CBalistar::Enter_Idle()
{
	m_ePhase = PHASE::IDLE;
	m_bAggro = false;
	Set_AnimBool(L"Detected", false);
	Set_AnimBool(L"Returning", false);
}

void CBalistar::Enter_Aim(_bool bReseed)
{
	if (true == bReseed)
	{
		m_bAimInit = false;
		m_bPitchInit = false;
	}

	m_ePhase = PHASE::AIM;
	m_bAggro = true;
	m_fAimTime = 0.f;
	m_fLoseTimer = 0.f;
	Set_AnimBool(L"Detected", true);
	Set_AnimBool(L"Returning", false);
}

void CBalistar::Enter_Return()
{
	m_ePhase = PHASE::RETURN;
	m_bAggro = false;
	m_fLoseTimer = 0.f;
	Set_AnimBool(L"Detected", false);
	Set_AnimBool(L"Returning", true);
}

void CBalistar::Begin_Attack()
{
	m_ePhase = PHASE::ATTACK;
	m_fPhaseTimer = 0.f;
	Request_AnimAction(L"ATTACK");
}

void CBalistar::Drive_Death()
{
	if (false == m_bDeathStarted)
	{
		m_bDeathStarted = true;
		m_bAggro = false;
		Set_AnimBool(L"Dead", true);
		Request_AnimAction(L"DEAD");
	}
}

void CBalistar::Fire_Projectile()
{
	_vector vOrigin, vForward;
	if (false == Get_GazeForward(vOrigin, vForward))
		vOrigin = m_pTransformCom->Get_State(STATE::POSITION);

	_vector vDir;
	_vector vAim;
	if (nullptr != m_pTarget && true == Get_TargetAimPos(vAim))
	{
		vDir = XMVectorSubtract(vAim, vOrigin);
		if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
			vDir = m_pTransformCom->Get_State(STATE::LOOK);
	}
	else
	{
		vDir = m_pTransformCom->Get_State(STATE::LOOK);
	}
	vDir = XMVector3Normalize(vDir);

	CProjectile::FIRE_DESC fd{};
	fd.eMode = CProjectile::MODE::SHOT;	
	XMStoreFloat3(&fd.vOrigin, vOrigin);
	XMStoreFloat3(&fd.vDir, vDir);
	fd.fSpeed = m_fBulletSpeed;
	fd.fRange = m_fBulletRange;
	fd.fRadius = m_fBulletRadius;
	fd.fDamage = m_fBulletDamage;
	fd.eHitType = HIT_TYPE::Heavy;
	fd.pOwner = this;
	fd.pTarget = m_pTarget;

	CMissile* pProjectile = m_pGameInstance->Get_PoolObject<CMissile>(&fd);
	if (nullptr == pProjectile)
		return;

	m_pGameInstance->Add_GameObject(static_cast<_uint>(m_pGameInstance->Get_CurrentLevelIndex()),
		TEXT("Layer_Projectile"), pProjectile);
}

_bool CBalistar::Perceive_Now()
{
	if (nullptr == m_pTarget)
		return false;

	CTransform* pTargetTransform = m_pTarget->Get_Transform();
	if (nullptr == pTargetTransform)
		return false;

	const _vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
	const _vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
	const _float fDist = XMVectorGetX(XMVector3Length(XMVectorSubtract(vTargetPos, vMyPos)));
	if (fDist > m_fDetectRange || fDist < m_fMinRange)
		return false;

	_float fArc;
	if (false == Get_AimArc(fArc))
		return false;
	if (fabsf(fArc) > m_fGazeHalfRad)
		return false;

	_float fPitch;
	if (true == Get_AimPitch(fPitch) && fabsf(fPitch) > m_fPitchHalfRad)
		return false;

	_vector vOrigin, vForward, vAim;
	if (false == Get_GazeForward(vOrigin, vForward) || false == Get_TargetAimPos(vAim))
		return false;

	return Has_LineOfSight(vOrigin, vAim);
}

_bool CBalistar::Get_GazeForward(_vector& OutOrigin, _vector& OutForward)
{
	if (nullptr == m_pModelCom || nullptr == m_pTransformCom)
		return false;

	const _float4x4* pMuzzle = m_pModelCom->Get_Bone_CombinedMatPtr(m_strFireBone.c_str());
	const _float4x4* pBase = m_pModelCom->Get_Bone_CombinedMatPtr(m_strGazeBaseBone.c_str());
	if (nullptr == pMuzzle || nullptr == pBase)
		return false;

	const _matrix matWorld = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
	const _vector vMuzzle = (XMLoadFloat4x4(pMuzzle) * matWorld).r[3];
	const _vector vBase = (XMLoadFloat4x4(pBase) * matWorld).r[3];

	_vector vFwd = XMVectorSubtract(vMuzzle, vBase);
	if (XMVectorGetX(XMVector3LengthSq(vFwd)) < 1e-8f)
		return false;

	OutOrigin = vMuzzle;
	OutForward = XMVector3Normalize(vFwd);
	return true;
}

_bool CBalistar::Get_TargetAimPos(_vector& OutAim)
{
	if (nullptr == m_pTarget)
		return false;

	CCharacter* pChar = static_cast<CCharacter*>(m_pTarget);
	if (nullptr != pChar && true == pChar->Get_LockOnWorldPosition(OutAim))
		return true;

	CTransform* pTransform = m_pTarget->Get_Transform();
	if (nullptr == pTransform)
		return false;

	OutAim = XMVectorAdd(pTransform->Get_State(STATE::POSITION), XMVectorSet(0.f, TARGET_AIM_HEIGHT, 0.f, 0.f));
	return true;
}

HRESULT CBalistar::Ready_Shooter()
{
	CShooter::SHOOTER_DESC ShooterDesc{};
	ShooterDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	ShooterDesc.strModelProtoTag = TEXT("Prototype_Component_Model_Shooter");
	ShooterDesc.strAnimFolderTag = TEXT("Shooter");
	ShooterDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	ShooterDesc.vOffset = m_vShooterOffset;
	ShooterDesc.vScale = m_vShooterScale;

	if (FAILED(Add_PartObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Shooter"),
		TEXT("Obj_Shooter"), &ShooterDesc)))
		return E_FAIL;

	m_pShooter = static_cast<CShooter*>(Get_PartObject(TEXT("Obj_Shooter")));
	if (nullptr == m_pShooter)
		return E_FAIL;

	return S_OK;
}

CBalistar* CBalistar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBalistar* pInstance = new CBalistar(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBalistar");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBalistar::Clone(void* pArg)
{
	CBalistar* pInstance = new CBalistar(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBalistar");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBalistar::Free()
{
	__super::Free();
}

#include "Weapon.h"
#include "Shader.h"	
#include "Model.h"
#include "DynamicRigidBody.h"
#include "Transform.h"
#include "GameInstance.h"
#include "Player.h"
#include "Engine_Utility.h"
#include "DebugCapsule.h"
#include "Effect.h"
#include "Status.h"
#include "AIObject.h"
#include "Weapon_Monster.h"
#include "Animator.h"

CWeapon::CWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CPartObject(pDevice, pContext)
{
}

CWeapon::CWeapon(const CWeapon& Prototype)
	:CPartObject(Prototype)
	, m_strTrailTopBoneName{ Prototype.m_strTrailTopBoneName }
	, m_strTrailBottomBoneName{ Prototype.m_strTrailBottomBoneName }
	, m_strEffectBoneName{ Prototype.m_strEffectBoneName }
	, m_iWeaponTypeID{ Prototype.m_iWeaponTypeID }
{
}

HRESULT CWeapon::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CWeapon::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	if (FAILED(__super::Initialize(pArg))) return E_FAIL;

	m_fCurDurability = MAX_DURABILITY;
	WEAPON_DESC* desc = static_cast<WEAPON_DESC*>(pArg);
	m_pOwner = desc->pOwner;
	m_pSocketBoneMatrix = desc->pSocketBoneMatrix;
	m_iLevelIndex = desc->iLevelIndex;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (FAILED(Ready_WeaponData()))
		return E_FAIL;

#ifdef _DEBUG
	//TODO : 하위에서 정의하기 
	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DebugDesc;

	DebugDesc.fHalfHeight = m_fSweepHalfHeight;
	DebugDesc.fRadius = m_fSweepRadius;
	DebugDesc.iSegment = 10;
	DebugDesc.vColor = _float4{ 1.f, 0.f, 0.f, 1.f };

	m_SweepState.pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DebugDesc);
	if (m_SweepState.pDebugCapsule == nullptr)
		return E_FAIL;

#endif 
	return S_OK;
}

void CWeapon::Priority_Update(_float fTimeDelta)
{
}

void CWeapon::Update(_float fTimeDelta)
{
}

void CWeapon::Late_Update(_float fTimeDelta)
{
	_matrix SocketMatrix = XMLoadFloat4x4(m_pSocketBoneMatrix);

	_vector vScale, vRotation, vTranslation;

	if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, SocketMatrix))
	{
		SocketMatrix = XMMatrixAffineTransformation(
			XMVectorSet(1.f, 1.f, 1.f, 0.f), XMVectorZero(), vRotation, vTranslation);
	}

	_matrix ModelWorldMatrix = XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()) * SocketMatrix;
	Compute_CombinedWorldMatrix(ModelWorldMatrix);

	Update_Collision(fTimeDelta);

	if (m_bVisibility)
	{
		m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
		m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
	}


#ifdef _DEBUG
	if (m_SweepState.pDebugCapsule && m_SweepState.bCollisionActive)
	{
		_bool bRender = m_pGameInstance->Get_DebugFlag();
		if (bRender) m_SweepState.pDebugCapsule->Late_Update(fTimeDelta);
	}
#endif

}

HRESULT CWeapon::Render()
{
	return S_OK;
}


void CWeapon::Begin_Sweep(SWEEP_STATE& State, AttackType eType, _uint iStep)
{
	if (!State.bCollisionActive)
	{
		State.bPrevPosValid = false;
		State.bCollisionActive = true;
		State.Targets.clear();
		State.pAction = Get_ActionData(eType, iStep);
	}
}

void CWeapon::End_Sweep(SWEEP_STATE& State)
{
	State.bCollisionActive = false;
	State.bPrevPosValid = false;
}

void CWeapon::Sweeping(SWEEP_STATE& State, _fmatrix BladeTop, _fmatrix BladeBot)
{
	if (State.bCollisionActive == false) return;

	const _vector vTop = BladeTop.r[3];
	const _vector vBot = BladeBot.r[3];
	const _vector vCenter = (vTop + vBot) * 0.5f;

	//칼날의 월드 방향벡터 계산 
	_vector vBladeDir = vTop - vBot;
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBladeDir)), 0.f))
	{
		vBladeDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	}
	vBladeDir = XMVector3Normalize(vBladeDir);

	//충돌 첫 프레임이라면 Sweep 보류
	if (State.bPrevPosValid == false)
	{
		XMStoreFloat3(&State.vPrevCenter, vCenter);
		State.bPrevPosValid = true;
		return;
	}

	const _vector vPrev = XMLoadFloat3(&State.vPrevCenter);
	_vector vDelta = vCenter - vPrev;
	XMStoreFloat3(&State.vPrevCenter, vCenter);

	const _float fDistance = XMVectorGetX(XMVector3Length(vDelta));
	if (fDistance <= 0.f) return;

	_vector vSweepDir = XMVector3Normalize(vDelta);

	//PX 캡슐 기본축 (로컬 +X 축) 이므로 현재 날 방향으로 회전 
	const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	const _float fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBladeDir));

	_vector vQuat;
	//이미 원하는 축에 맞춰졌다면 
	if (IsEqual(fDot, 1.f))
		vQuat = XMQuaternionIdentity();
	//반대 방향이라면 반대로 회전 
	else if (IsEqual(fDot, -1.f))
		vQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
	else
	{
		//회전시킬 기준 축 구하기 
		const _vector vAxis = XMVector3Normalize(XMVector3Cross(vAxisX, vBladeDir));
		vQuat = XMQuaternionRotationAxis(vAxis, acosf(fDot));
	}

#ifdef _DEBUG
	if (State.pDebugCapsule)
	{
		State.pDebugCapsule->Update_Pose(vPrev, vQuat);
	}
#endif

	_bool bHit = false;
	_float fDamage = 0.f;

	//충돌 정보를 받을 데이터 
	CollisionResult tResult{};
	if (m_pGameInstance->Sweep_Capsule(m_fSweepRadius, m_fSweepHalfHeight, vPrev, vQuat, vSweepDir, fDistance, m_eTargetGroup, tResult))
	{
		CGameObject* pTarget = tResult.pOtherActor;
		if (pTarget == nullptr) return;

		HitInfo hitInfo;

		const _float fBaseDamage = ((State.pAction == nullptr) ? BASE_DAMAGE : State.pAction->fDamage);
		hitInfo.fDamage = (m_fDamageVariance > 0.f) ? Random(fBaseDamage * (1.f - m_fDamageVariance), fBaseDamage * (1.f + m_fDamageVariance)) : fBaseDamage;
		hitInfo.fGroggy = ((State.pAction == nullptr) ? BASE_GROGGY : State.pAction->fGroggy);
		hitInfo.pOtherActor = m_pOwner;
		hitInfo.pDamangeCauser = this;
		hitInfo.vHitPosition = tResult.tSweepData.vHitPosition;
		hitInfo.vHitNormal = tResult.tSweepData.vHitNormal;
		hitInfo.eHitType = ((State.pAction == nullptr) ? HIT_TYPE::Normal : State.pAction->eHitType);


		if (State.Targets.find(pTarget) == State.Targets.end())
		{
			if (pTarget->Take_Damage(hitInfo) == false)
				return;

			State.Targets.insert(pTarget);
			bHit = true;

			_uint iHitGroup = 0;
			if (nullptr != tResult.tSweepData.pHitShape)
				iHitGroup = tResult.tSweepData.pHitShape->getQueryFilterData().word0;

			if (iHitGroup & ETOUI(FILTER_GROUP::MONSTEROBJECT))
			{
				CWeapon_Monster* pMonsterPart = static_cast<CWeapon_Monster*>(pTarget);

				if (nullptr != pMonsterPart->Get_Owner())
					State.Targets.insert(pMonsterPart->Get_Owner());
			}
			else if (iHitGroup & ETOUI(FILTER_GROUP::MONSTER))
			{
				fDamage = hitInfo.fDamage;
				CAIObject* pAIObject = static_cast<CAIObject*>(pTarget);

				if (hitInfo.eHitType == HIT_TYPE::Fatal)
				{
					CStatusComponent* pStatusCom = m_pOwner->Get_StatusComponent();
					if (pStatusCom)
					{
						if (pStatusCom->Is_FatalLastHit())
						{
							pAIObject->Set_FatalMode(false);

							m_pGameInstance->Play_Effect("Effect_Player_Fatal_Attack", hitInfo.vHitPosition);
						}
					}
				}
				else if (hitInfo.eHitType == HIT_TYPE::Charge)
				{
					_bool bIsGroggyFull = pAIObject->Is_Groggy_Full();
					if (bIsGroggyFull)
						m_pGameInstance->Play_Effect("Effect_Player_Groggy_Attack", hitInfo.vHitPosition);
				}
			}
			Spawn_SwingFX(XMLoadFloat3(&hitInfo.vHitPosition), vSweepDir, ((State.pAction == nullptr) ? ATTACK_STYLE::Slash : State.pAction->eAttackStyle));
		}
	}

	if (bHit)
	{
		Consume_Durability();
		m_pOwner->On_Attack(fDamage);
	}
}

void CWeapon::Drive_ActionWindows(CAnimator* pAnimator)
{
	_bool bHit = pAnimator->Get_GameplayFlags().bHitActive;
	if (bHit == m_bHitWindow) return;
	m_bHitWindow = bHit;

	if (bHit)
	{
		On_Collision(m_eCurAttackType, m_iCurComboStep);
		Set_TrailActive(true, ETOUI(m_eCurAttackType), m_iCurComboStep);
	}
	else
	{
		Off_Collision();
		Set_TrailActive(false);
	}
}

void CWeapon::On_Unequipped()
{
	m_bHitWindow = false;
	m_bSwapping = false;
	Off_Collision();
	Set_TrailActive(false);
	Set_AttackContext(AttackType::End, 0);
}

//애니메이션에서 출력하고있는 타입과 콤보Idx를 꽂아주기 
void CWeapon::On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePieceType)
{
	Begin_Sweep(m_SweepState, eType, iStep);
}

void CWeapon::Off_Collision(PIECE_TYPE ePieceType)
{
	End_Sweep(m_SweepState);
}

void CWeapon::Update_Collision(_float fTimeDelta)
{
	if (!m_SweepState.bCollisionActive) return;

	_matrix BladeTop, BladeBot;
	if (FAILED(Get_TrailMatrix(BladeTop, BladeBot))) return;
	Sweeping(m_SweepState, BladeTop, BladeBot);
}

_bool CWeapon::Consume_Durability()
{
	if (m_fCurDurability <= 0.f) return false;
	m_fCurDurability = max(0.f, m_fCurDurability - DURABILITY_PER_HIT);
	return true;
}

void CWeapon::Set_FullDurability()
{
	m_fCurDurability = MAX_DURABILITY;
}

void CWeapon::Regen_Durability(_float fAmountPerSec, _float fDeltaTime)
{
	if (m_fCurDurability >= MAX_DURABILITY) return;
	m_fCurDurability = min(MAX_DURABILITY, m_fCurDurability + fAmountPerSec * fDeltaTime);
}


const ActionData* CWeapon::Get_ActionData(AttackType eAttackType, _uint iStep)
{
	auto iter = m_tWeaponData.AttackSteps.find(eAttackType);
	if (iter == m_tWeaponData.AttackSteps.end()) return nullptr;
	if (iStep >= iter->second.size()) return nullptr;

	return &iter->second[iStep];
}

HRESULT CWeapon::Get_TrailBoneMatPtr(const _float4x4*& pOutTopBoneMat, const _float4x4*& pOutBottomBoneMat)
{
	if (m_iTrailTopBoneIndex == -1 || m_iTrailBottomBoneIndex == -1) return E_FAIL;

	pOutTopBoneMat = Get_Bone_CombinedMatPtr(m_iTrailTopBoneIndex);
	pOutBottomBoneMat = Get_Bone_CombinedMatPtr(m_iTrailBottomBoneIndex);
	if (pOutTopBoneMat == nullptr || pOutBottomBoneMat == nullptr) return E_FAIL;

	return S_OK;
}

HRESULT CWeapon::Get_TrailMatrix(_matrix& OutTopMatrix, _matrix& OutBottomMatrix)
{
	if (m_iTrailTopBoneIndex == -1 || m_iTrailBottomBoneIndex == -1) return E_FAIL;

	if (FAILED(Get_BoneWorldMatrix(m_iTrailTopBoneIndex, OutTopMatrix))) return E_FAIL;
	if (FAILED(Get_BoneWorldMatrix(m_iTrailBottomBoneIndex, OutBottomMatrix))) return E_FAIL;

	return S_OK;
}

HRESULT CWeapon::Get_EffectBoneMatrix(_matrix& OutEffectMatrix)
{
	if (m_iEffectBoneIndex == -1) return E_FAIL;
	if (FAILED(Get_BoneWorldMatrix(m_iEffectBoneIndex, OutEffectMatrix))) return E_FAIL;

	return S_OK;
}

HRESULT CWeapon::Get_EffectBoneMatPtr(const _float4x4*& pOutBoneMat)
{
	if (m_iEffectBoneIndex == -1) return E_FAIL;

	pOutBoneMat = Get_Bone_CombinedMatPtr(m_iEffectBoneIndex);
	if (pOutBoneMat == nullptr) return E_FAIL;
	return S_OK;
}

void CWeapon::Set_Dissolve(_float fAmount, const _float4& vColor)
{
	m_fDissolveAmount = fAmount;
	m_vDissolveColor = vColor;
}

//_float CWeapon::Get_HitTypeDamageAlpha(HIT_TYPE eHitType)
//{
//	switch (eHitType)
//	{
//	case HIT_TYPE::Normal:			return 5.f;
//	case HIT_TYPE::NormalUnGuard:	return 8.f;
//	case HIT_TYPE::Heavy:			return 15.f;
//	case HIT_TYPE::HeavyUnGuard:	return 18.f;
//	case HIT_TYPE::KnockBack:		return 10.f;
//	case HIT_TYPE::KnockDown:		return 12.f;
//	case HIT_TYPE::Charge:			return 10.f;
//	case HIT_TYPE::Fury:			return 15.f;
//	case HIT_TYPE::Fatal:			return 20.f;
//	case HIT_TYPE::FatalEnd:		return 20.f;
//	default:						return 0.f;
//	}
//	return 0.f;
//}

void CWeapon::Set_TrailActive(_bool bActive, _uint eType, _uint iComboStep)
{
	if (bActive && eType != ETOUI(AttackType::End) && m_pChannel_TrailFX == nullptr)
	{
		if (m_FXNames[eType][iComboStep].empty()) return;

		const _float4x4* pTrailTop = nullptr;
		const _float4x4* pTrailBottom = nullptr;

		Get_TrailBoneMatPtr(pTrailTop, pTrailBottom);
		m_pChannel_TrailFX = m_pGameInstance->Play_Effect(m_FXNames[eType][iComboStep], pTrailTop, pTrailBottom, Get_CombinedWorldMatrixPtr());
	}
	else if (!bActive && m_pChannel_TrailFX != nullptr)
	{
		m_pChannel_TrailFX->Stop();
		m_pChannel_TrailFX = nullptr;
	}
}

_fmatrix CWeapon::Make_EffectStampMatrix(_fvector vPos, _fvector vSlashDir)
{
	_vector vCamPos = XMLoadFloat4(m_pGameInstance->Get_CamPosition());
	_vector vLook = XMVector3Normalize(vCamPos - vPos);
	_vector vSlash = XMVector3Normalize(vSlashDir);

	_vector vTempRight = vSlash - XMVector3Dot(vSlash, vLook) * vLook;
	_vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vTempRight));
	_vector vRight = XMVector3Cross(vUp, vLook);

	XMMATRIX RotMatrix = XMMATRIX(
		vRight,
		vUp,
		vLook,
		XMVectorSetW(vPos, 1.f)
	);
	return RotMatrix;
}

//찌르기는 Y축 정렬 전제
_fmatrix CWeapon::Make_ScreenAlignedMatrix(_fvector vPos, _fvector vThrustDir)
{
	_vector vUp = XMVector3Normalize(vThrustDir);
	_vector vTempForward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	//TempForward와 평행이라면 축 교체 
	if (fabsf(XMVectorGetX(XMVector3Dot(vUp, vTempForward))) > 0.999f)
		vTempForward = XMVectorSet(1.f, 0.f, 0.f, 0.f);

	_vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vTempForward));
	_vector vForward = XMVector3Cross(vRight, vUp);

	return XMMATRIX(
		vRight,
		vUp,
		vForward,
		XMVectorSetW(vPos, 1.f)
	);

}


void CWeapon::Spawn_SwingFX(_fvector vFXPos, _fvector vSwingDir, ATTACK_STYLE eStyle)
{
	if (XMVector3Equal(vSwingDir, XMVectorZero())) return;

	if (eStyle == ATTACK_STYLE::Slash)
	{
		_matrix RotationSlashDirMat = Make_EffectStampMatrix(vFXPos, vSwingDir);
		XMStoreFloat4x4(&m_EffectMatrixPool[m_iEffectMatrixPoolIndex], RotationSlashDirMat);
		m_pGameInstance->Play_Effect("Effect_Player_Attack_Swing", nullptr, &m_EffectMatrixPool[m_iEffectMatrixPoolIndex], _float3(0.f, 0.f, 0.f));
		m_pGameInstance->Play_Sound("SE_PC_SK_Hit_Melamine_Slice_S_02", SOUND_PLAYER, 0.3f, false);
	}
	else if (eStyle == ATTACK_STYLE::Thrust)
	{
		const _float4x4* pBoneMat = nullptr;
		if (FAILED(Get_EffectBoneMatPtr(pBoneMat))) return;

		const _float4x4* pMatPos = Get_CombinedWorldMatrixPtr();
		m_pGameInstance->Play_Effect("Effect_Player_Attack_Thrust", pBoneMat, pMatPos, _float3(-3.f, 0.f, 0.f));
		m_pGameInstance->Play_Sound("SE_PC_SK_Hit_Melamine_Stab_L_02", SOUND_PLAYER, 0.3f, false);
	}

	m_iEffectMatrixPoolIndex = (m_iEffectMatrixPoolIndex + 1) % EFFECT_MATRIX_POOL_SIZE;
}


const _float4x4* CWeapon::Get_Bone_CombinedMatPtr(_uint iBoneIndex)
{
	if (m_pModelCom == nullptr) return nullptr;
	return m_pModelCom->Get_Bone_CombinedMatPtr(iBoneIndex);
}

const _float4x4* CWeapon::Get_Bone_CombinedMatPtr(const _char* pBoneName)
{
	if (m_pModelCom == nullptr) return nullptr;
	return m_pModelCom->Get_Bone_CombinedMatPtr(pBoneName);
}

HRESULT CWeapon::Get_BoneWorldMatrix(_uint iBoneIndex, _matrix& OutMatrix)
{
	if (m_pModelCom == nullptr) return E_FAIL;

	const _float4x4* pBoneCombinedMat = m_pModelCom->Get_Bone_CombinedMatPtr(iBoneIndex);
	if (pBoneCombinedMat == nullptr) return E_FAIL;

	_matrix boneLocal = XMLoadFloat4x4(pBoneCombinedMat);
	OutMatrix = boneLocal * XMLoadFloat4x4(&m_CombinedWorldMatrix);
	return S_OK;
}


void CWeapon::Free()
{
	__super::Free();
	Safe_Release(m_pModelCom);

#ifdef _DEBUG
	Safe_Release(m_SweepState.pDebugCapsule);
#endif
}

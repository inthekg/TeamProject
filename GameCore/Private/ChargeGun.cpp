#include "ChargeGun.h"
#include "Player.h"
#include "GameInstance.h"
#include "AnimDriver.h"
#include "SpringArm.h"
#include "Status.h"
#include "Transform.h"
#include "AimComponent.h"
#include "MasterModel.h"
#include "ChargeBomb.h"
#include "AIObject.h"

CChargeGun::CChargeGun(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CLegionArm{ pDevice, pContext }
{
}

HRESULT CChargeGun::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CChargeGun::Initialize : pArg is nullptr");
		return E_FAIL;
	}

	CLegionArm::LEGIONARM_DESC* pDesc = static_cast<CLegionArm::LEGIONARM_DESC*>(pArg);
	pDesc->strModelTag = L"Prototype_Component_Model_Player_ChargeGun";

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_eType = LegionArmType::ChargeGun;

	CAimBoneChain::AIM_PROFILE AimProfile = {};
	AimProfile.Bones =
	{
		{ "Bip001-Spine", 0.20f },
		{ "Bip001-Spine1", 0.30f },
		{ "Bip001-Spine2", 0.50f }
	};
	AimProfile.fMaxYaw = AimProfile.fMaxPitchUp = AimProfile.fMaxPitchDown = XMConvertToRadians(AIM_CHAIN_MAX_ANGLE);

	m_iAimProfile = m_pAimCom->Register_AimProfile(AimProfile);
	if (m_iAimProfile == CAimComponent::INVALID_PROFILE)
	{
		MSG_BOX("CChargeGun::Initialize : Failed to Register AimProfile");
		return E_FAIL;
	}
	
	return S_OK;
}

void CChargeGun::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	if (m_eState == ChargeGunState::Idle) return;

	m_fStateTime += fTimeDelta;

	if(!m_bAnimEntered && m_pAnimDriver->Is_ActionActive(AnimAction::ChargeGun))
	{
		m_bAnimEntered = true;
	}

	_bool bExitFlag = (m_bAnimEntered && !m_pAnimDriver->Is_ActionActive(AnimAction::ChargeGun));
	bExitFlag |= (!m_bAnimEntered && m_fStateTime > STATE_TIMEOUT);

	if (bExitFlag)
	{
		End_ChargeGun();
		return;
	}

	if (m_eState == ChargeGunState::Enter || m_eState == ChargeGunState::Aim || m_eState == ChargeGunState::Fire)
	{
		Update_YawFollow(fTimeDelta);
		m_pAimCom->Request_AimPoseToCamera(m_iAimProfile);
	}

	if (m_eState == ChargeGunState::Aim && m_bFirePending)
	{
		m_bFirePending = false;
		Enter_Fire();
	}
}

void CChargeGun::On_AnimEvent(LegionArmEvent eEvent)
{
	if (eEvent == LegionArmEvent::ChargeGun_Accepted)
	{
		if (m_eState == ChargeGunState::Idle)
			Begin_ChargeGun();
		return;
	}

	if (m_eState == ChargeGunState::Idle)
		return;

	switch (eEvent)
	{
	case GameCore::LegionArmEvent::ChargeGun_AimReady:
	{
		Enter_Aim();
		break;
	}
	case GameCore::LegionArmEvent::ChargeGun_Fire:
	{
		if (m_eState == ChargeGunState::Fire)
			Fire_Bullet();
		break;
	}
	case GameCore::LegionArmEvent::ChargeGun_Exit:
	{
		Enter_Exit();
		break;
	}
	default:
		break;
	}

}

// Aim중에 키가 다시 눌리면 발사. idle때 눌리면 Begin ChargeGun 
void CChargeGun::Use()
{
	Use_Internal(false);
}

void CChargeGun::Use_Upgrade()
{
	Use_Internal(true);
}

void CChargeGun::Use_Internal(_bool bUgrade)
{
	if (m_pOwner == nullptr)
		return;

	switch (m_eState)
	{
	case GameCore::ChargeGunState::Idle:
	{
		//이미 차지건 진행중 
		if (m_pAnimDriver->Is_ActionActive(AnimAction::ChargeGun))
			return;

		m_bFirePending = false;
		m_bUpgradeShot = bUgrade;
		Request_ChargeGun();

		break;
	}
	case GameCore::ChargeGunState::Enter:
	{
		m_bFirePending = true;
		break;
	}
	case GameCore::ChargeGunState::Aim:
	{
		Enter_Fire();
		break;
	}
	default:
		break;
	}
}

void CChargeGun::Request_Cancel()
{
	if (m_eState == ChargeGunState::Enter || m_eState == ChargeGunState::Aim)
		Enter_Exit();
}

void CChargeGun::Cancel()
{
	if (m_eState == ChargeGunState::Idle)
		return;

	End_ChargeGun();
}

void CChargeGun::Request_ChargeGun()
{
	m_fStateTime = 0.f;	

	m_pAnimDriver->Set_Param(PARAM_CHARGEGUN_FIRE, false);
	m_pAnimDriver->Play_Action(AnimAction::ChargeGun);
}

void CChargeGun::Begin_ChargeGun()
{
	m_eState = ChargeGunState::Enter;
	m_fStateTime = 0.f;
	m_bAnimEntered = false;
	
	Set_CrosshairVisible(true);

	if (m_pSpringArm)
	{
		CSpringArm::SPRING_ADJUST_DESC AdjustDesc;
		AdjustDesc.fBlendInTime = CAM_BLEND_IN_TIME;
		AdjustDesc.fBlendOutTime = CAM_BLEND_OUT_TIME;
		AdjustDesc.vLocalOffset = _float3{ CAM_AIM_RIGHT_OFFSET, CAM_AIM_UP_OFFSET, 0.f };
		AdjustDesc.fArmLength = CAM_ARM_LENGTH;
		m_pSpringArm->Begin_Adjust(AdjustDesc);
	}
}

void CChargeGun::Enter_Aim()
{
	if(m_eState != ChargeGunState::Enter)
		return;

	m_eState = ChargeGunState::Aim;
	m_fStateTime = 0.f;
}

void CChargeGun::Enter_Fire()
{
	if (m_eState != ChargeGunState::Aim)
		return;
	
	m_eState = ChargeGunState::Fire;
	m_fStateTime = 0.f;

	Set_CrosshairVisible(false);

	m_pAnimDriver->Set_Param(PARAM_CHARGEGUN_FIRE, true);
}

void CChargeGun::Enter_Exit()
{
	End_ChargeGun();
}

void CChargeGun::Fire_Bullet()
{
	m_pGameInstance->Play_Effect(MUZZLE_EFFECT_NAME, m_pOwner->Get_Model(), MUZZLE_OWNER_BONE_NAME, m_pOwner->Get_Transform()->Get_WorldMatrixPtr());

	_vector vStart = {}, vDir = {};
	Compute_CameraRay(vStart, vDir);

	FILTER_GROUP eTargetFilter = static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::MONSTER) | ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));

	CollisionResult Result;
	_bool bHit = m_pGameInstance->Sweep_Sphere(SWEEP_RADIUS, vStart, vDir, FIRE_RANGE, eTargetFilter, Result, PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC);

	CChargeBomb::CHARGEBOMB_DESC Desc{};
	Desc.pOwner = m_pOwner;
	Desc.fDelay = BOMB_DELAY;
	Desc.fRadius = BOMB_RADIUS;
	Desc.fDamage = BOMB_DAMAGE;
	Desc.bUpgrade = m_bUpgradeShot;
	XMStoreFloat3(&Desc.vHitDir, vDir);

	if (bHit)
	{
		Desc.vPosition = Result.tSweepData.vHitPosition;
		Desc.vNormal = Result.tSweepData.vHitNormal;
		//지형이면 nullptr
		Desc.pAttachTarget = dynamic_cast<CAIObject*>(Result.pOtherActor);
	}
	else
	{
		//최대거리에서 폭발 
		XMStoreFloat3(&Desc.vPosition, vStart + vDir * FIRE_RANGE);
		Desc.fDelay = 0.f;
	}

	CChargeBomb* pBomb = m_pGameInstance->Get_PoolObject<CChargeBomb>(&Desc);
	//레이어 이름이 Layer_Monster 보다 뒤에 오는 이름이여야 몬스터가 이번 프레임 본을 갱신한 뒤에 읽음
	if (pBomb)
		m_pGameInstance->Add_GameObject(ETOUI(m_pGameInstance->Get_CurrentLevelIndex()), TEXT("Layer_PlayerBomb"), pBomb);

}

//Yaw는 몸 전체가 따라감 
void CChargeGun::Update_YawFollow(_float fTimeDelta)
{
	_float fCamYaw = m_pSpringArm->Get_HorizonValue();
	_vector vTargetQuat = XMQuaternionRotationRollPitchYaw(0.f, fCamYaw, 0.f);

	_float fRatio = 1.f - expf(-AIM_YAW_FOLLOW_RATE * fTimeDelta);
	m_pOwner->Get_Transform()->Smooth_Turn(vTargetQuat, fRatio);
}

void CChargeGun::End_ChargeGun()
{
	if (m_pSpringArm) m_pSpringArm->End_Adjust();
	Set_CrosshairVisible(false);

	m_pAnimDriver->Set_Param(PARAM_CHARGEGUN_FIRE, false);

	m_eState = ChargeGunState::Idle;
	m_bAnimEntered = false;
	m_bFirePending = false;
}



CChargeGun* CChargeGun::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CChargeGun* pInstance = new CChargeGun(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CChargeGun");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CChargeGun::Free()
{
	__super::Free();
}

#include "GreatSword.h"
#include "Shader.h"	
#include "Model.h"
#include "DynamicRigidBody.h"
#include "Transform.h"
#include "GameInstance.h"
#include "Status.h"

CGreatSword::CGreatSword(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CSkeletalWeapon{ pDevice, pContext }
{
}

CGreatSword::CGreatSword(const CGreatSword& Prototype)
	: CSkeletalWeapon{ Prototype }
{
}

HRESULT CGreatSword::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_iWeaponTypeID = 1;

	m_FXNames[ETOUI(AttackType::Light)][1] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::Heavy)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::ChargeHeavy)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::Fatal)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::DashLight)][0] = "Effect_Player_Normal_Trail";

	m_FXNames[ETOUI(AttackType::Fable)][0] = "Effect_Player_TwoHand_Pable_Trail_1";
	m_FXNames[ETOUI(AttackType::Fable)][1] = "Effect_Player_TwoHand_Pable_Trail_1";
	m_FXNames[ETOUI(AttackType::Fable)][2] = "Effect_Player_TwoHand_Pable_Trail_2";

	m_strTrailTopBoneName = "FX_WP_Trail_R_1_1";
	m_strTrailBottomBoneName = "FX_WP_Trail_R_1_3";
	m_strEffectBoneName = "FX_WP_Trail_R_1_2";


	return S_OK;
}

HRESULT CGreatSword::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_fSweepRadius = { 0.25f };
	m_fSweepHalfHeight = { 0.65f };

	return S_OK;
}

void CGreatSword::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CGreatSword::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CGreatSword::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

void CGreatSword::Execute_FableArts(_uint iStep)
{
}

HRESULT CGreatSword::Ready_Components()
{
	if (FAILED(__super::Add_Component(m_iLevelIndex, L"Prototype_Component_Model_GreatSword",
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	//애니메이션이 없는 모델이면 여기서 한번 호출하기 
	m_pModelCom->Update_Bones();

	m_iTrailTopBoneIndex = m_pModelCom->Get_BoneIndex(m_strTrailTopBoneName.c_str());
	m_iTrailBottomBoneIndex = m_pModelCom->Get_BoneIndex(m_strTrailBottomBoneName.c_str());
	m_iEffectBoneIndex = m_pModelCom->Get_BoneIndex(m_strEffectBoneName.c_str());

	if (m_iTrailTopBoneIndex == -1 || m_iTrailBottomBoneIndex == -1)
	{
		MSG_BOX("CGreatSword : Trail Bone Not Found");
		return E_FAIL;
	}

	if (m_iEffectBoneIndex == -1)
	{
		MSG_BOX("CGreatSword : Effect Bone Not Found");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGreatSword::Ready_WeaponData()
{
	m_tWeaponData.eType = WeaponType::TwoHand;

	//TODO : 애니메이터 돌려보고 Profile, Trigger 제거할지 결정 
	//eCategory, profile, strAnimTrigger, fStaminaCost, fDamage
	m_fDamageVariance = 0.10f;

	ActionData Light1ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"LightAttack1", 20.f, 600.f , HIT_TYPE::Normal, ATTACK_STYLE::Thrust, 10.f };
	ActionData Light2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"LightAttack2", 20.f, 600.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };

	ActionData HeavyActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"HeavyAttack1", 20.f, 900.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };
	ActionData Heavy2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"HeavyAttack2", 20.f, 900.f , HIT_TYPE::Normal, ATTACK_STYLE::Thrust, 20.f };

	ActionData ChargeActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"CharageAttack1", 30.f, 1500.f , HIT_TYPE::Charge, ATTACK_STYLE::Slash, 30.f };

	ActionData DashLightActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"DashLightAttack", 20.f, 800.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };
	ActionData DashHeavyActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"DashHeavyAttack", 20.f, 800.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };

	ActionData FatalActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FatalAttack1", 30.f, 2200.f , HIT_TYPE::Fatal, ATTACK_STYLE::Slash, 0.f };

	ActionData Fable1ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FableAttack1", 20.f, 1300.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };
	ActionData Fable2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FableAttack2", 20.f, 1300.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };
	ActionData Fable3ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Armored , L"FableAttack3", 20.f, 1800.f , HIT_TYPE::Charge, ATTACK_STYLE::Thrust, 30.f };

	//패링 페이블
	ActionData FableExActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Free , L"FableAttackEx", 0.f, 0.f , HIT_TYPE::End, ATTACK_STYLE::End, 0.f };

	m_tWeaponData.AttackSteps[AttackType::Light] =
	{
		Light1ActionData, Light2ActionData
	};

	m_tWeaponData.AttackSteps[AttackType::Heavy] =
	{
		HeavyActionData,Heavy2ActionData
	};

	m_tWeaponData.AttackSteps[AttackType::ChargeHeavy] =
	{
		ChargeActionData
	};

	m_tWeaponData.AttackSteps[AttackType::DashLight] =
	{
		DashLightActionData
	};

	m_tWeaponData.AttackSteps[AttackType::DashHeavy] =
	{
		DashHeavyActionData
	};

	m_tWeaponData.AttackSteps[AttackType::Fatal] =
	{
		FatalActionData
	};

	m_tWeaponData.AttackSteps[AttackType::Fable] =
	{
		Fable1ActionData, Fable2ActionData, Fable3ActionData
	};

	m_tWeaponData.AttackSteps[AttackType::FableEX] =
	{
		FableExActionData
	};


	return S_OK;
}

CGreatSword* CGreatSword::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CGreatSword* pInstance = new CGreatSword(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CGreateSword");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CGreatSword::Clone(void* pArg)
{
	CGreatSword* pInstance = new CGreatSword(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGreateSword");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CGreatSword::Free()
{
	__super::Free();
}

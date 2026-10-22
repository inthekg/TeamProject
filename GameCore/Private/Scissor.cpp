#include "Scissor.h"
#include "Shader.h"	
#include "Model.h"
#include "DynamicRigidBody.h"
#include "Transform.h"
#include "GameInstance.h"
#include "Status.h"
#include "Player.h"
#include "DebugCapsule.h"
#include "AIObject.h"
#include "Animator.h"
#include "Effect.h"

CScissor::CScissor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CSplitWeapon{ pDevice, pContext }
{

}

CScissor::CScissor(const CScissor& Prototype)
	: CSplitWeapon{ Prototype }
{
}

HRESULT CScissor::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_iWeaponTypeID = 1;

	m_FXNames[ETOUI(AttackType::Light)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::Light)][1] = "Effect_Player_Normal_Trail";

	m_FXNames[ETOUI(AttackType::Heavy)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::Heavy)][1] = "Effect_Player_Normal_Trail";

	m_FXNames[ETOUI(AttackType::DashLight)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::ChargeHeavy)][0] = "Effect_Player_Normal_Trail";
	m_FXNames[ETOUI(AttackType::Fatal)][0] = "Effect_Player_Normal_Trail";

	m_FXNames[ETOUI(AttackType::Fable)][0] = "Effect_Player_TwoHand_Pable_Trail_1";
	m_FXNames[ETOUI(AttackType::Fable)][1] = "Effect_Player_TwoHand_Pable_Trail_1";
	m_FXNames[ETOUI(AttackType::Fable)][2] = "Effect_Player_TwoHand_Pable_Trail_1";

	m_FXNames[ETOUI(AttackType::FableEX)][0] = "Effect_Repair";

	//m_strTrailTopBoneName = "FX_WP_Trail_R_1_1";
	m_strTrailTopBoneName = "Weapon_Smash";
	m_strTrailBottomBoneName = "FX_WP_Trail_R_1_2";
	m_strEffectBoneName = "Weapon_Repulse";

	return S_OK;
}

HRESULT CScissor::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	if (FAILED(CPartObject::Initialize(pArg))) return E_FAIL;

	m_fCurDurability = MAX_DURABILITY;

	WEAPON_DESC* desc = static_cast<WEAPON_DESC*>(pArg);

	m_pOwner = desc->pOwner;
	m_pSocketBoneMatrix = desc->pSocketBoneMatrix;
	m_iLevelIndex = desc->iLevelIndex;

	m_fSweepRadius = { 0.25f };
	m_fSweepHalfHeight = { 0.6f };

	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (FAILED(Ready_WeaponData()))
		return E_FAIL;

	for (_int i = 0; i < PIECE_END; ++i)
	{
		CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DebugDesc;
		DebugDesc.fHalfHeight = m_fSweepHalfHeight;
		DebugDesc.fRadius = m_fSweepRadius;
		DebugDesc.iSegment = 10;
		DebugDesc.vColor = _float4{ 1.f, 0.f, 0.f, 1.f };

		m_Pieces[i].Sweep.pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DebugDesc);
		if (m_Pieces[i].Sweep.pDebugCapsule == nullptr)
			return E_FAIL;
	}

	return S_OK;
}



void CScissor::Execute_FableArts(_uint iStep)
{
	//버프 구현 
}

HRESULT CScissor::Ready_Components()
{
	//피스별 모델 로드 
	if (FAILED(__super::Add_Component(m_iLevelIndex, L"Prototype_Component_Model_Scissor_Merge",
		TEXT("Com_Model_M"), reinterpret_cast<CComponent**>(&m_Pieces[PIECE_MERGE].pModel))))
		return E_FAIL;

	if (FAILED(__super::Add_Component(m_iLevelIndex, L"Prototype_Component_Model_Scissor_Left",
		TEXT("Com_Model_L"), reinterpret_cast<CComponent**>(&m_Pieces[PIECE_LEFT].pModel))))
		return E_FAIL;

	if (FAILED(__super::Add_Component(m_iLevelIndex, L"Prototype_Component_Model_Scissor_Right",
		TEXT("Com_Model_R"), reinterpret_cast<CComponent**>(&m_Pieces[PIECE_RIGHT].pModel))))
		return E_FAIL;

	const _float4x4* pLeftSocketMat = m_pOwner->Get_Bone_CombinedMatPtr(m_pOwner->Get_LeftHandBoneName().c_str());
	const _float4x4* pRightSocketMat = m_pOwner->Get_Bone_CombinedMatPtr(m_pOwner->Get_RightHandBoneName().c_str());

	for (int i = 0;i < PIECE_END; ++i)
	{
		const _float4x4* pSocketMat = (i == PIECE_LEFT) ? pLeftSocketMat : pRightSocketMat;
		string strTopBoneName = (i == PIECE_MERGE) ? "FX_WP_Trail_R_1_1" : "Weapon_Smash";

		if (FAILED(Setup_Piece(static_cast<PIECE_TYPE>(i), m_Pieces[i].pModel, pSocketMat, strTopBoneName, m_strTrailBottomBoneName, m_strEffectBoneName)))
		{
			MSG_BOX("CScissor : Setup_Piece Failed");
			return E_FAIL;
		}
		m_Pieces[i].pModel->Update_Bones();

	}

	return S_OK;
}

HRESULT CScissor::Ready_WeaponData()
{
	m_tWeaponData.eType = WeaponType::OneHand;

	m_fDamageVariance = 0.15f;

	ActionData Light1ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"LightAttack1", 20.f, 700.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };
	ActionData Light2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"LightAttack2", 20.f, 700.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };

	ActionData HeavyActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"HeavyAttack1", 20.f, 1100.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };
	ActionData Heavy2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"HeavyAttack2", 20.f, 1100.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };

	ActionData ChargeActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"CharageAttack1", 30.f, 1900.f , HIT_TYPE::Charge, ATTACK_STYLE::Slash, 30.f };

	ActionData DashLightActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"DashLightAttack", 20.f, 1000.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };
	ActionData DashHeavyActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"DashHeavyAttack", 20.f, 1000.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 10.f };

	ActionData FatalActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FatalAttack1", 30.f, 2800.f , HIT_TYPE::Fatal, ATTACK_STYLE::Slash, 0.f };

	ActionData Fable1ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FableAttack1", 20.f, 1600.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };

	ActionData Fable2ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Attack , L"FableAttack2", 20.f, 1600.f , HIT_TYPE::Normal, ATTACK_STYLE::Slash, 20.f };

	ActionData Fable3ActionData
		= { ActionCategory::Attack, CStatusComponent::g_Profile_Armored , L"FableAttack3", 20.f, 2300.f , HIT_TYPE::Charge, ATTACK_STYLE::Slash, 30.f };

	ActionData FableEXActionData
		= { ActionCategory::Action, CStatusComponent::g_Profile_Armored , L"FableEX", 20.f, 0.f , HIT_TYPE::End, ATTACK_STYLE::End, 0.f };

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
		FableEXActionData
	};

	return S_OK;
}

void CScissor::Drive_ActionWindows(CAnimator* pAnimator)
{
	Set_Split(pAnimator->Is_InCustomWindow(WIN_NAME_WEAPON_SPLIT));

	Drive_PieceHit(PIECE_MERGE, pAnimator->Get_GameplayFlags().bHitActive);
	Drive_PieceHit(PIECE_LEFT, pAnimator->Is_InCustomWindow(WIN_NAME_HIT_ON_LEFT));
	Drive_PieceHit(PIECE_RIGHT, pAnimator->Is_InCustomWindow(WIN_NAME_HIT_ON_RIGHT));

	if (pAnimator->Is_InCustomWindow(WIN_NAME_WEAPON_EFFECT))
	{
		if (m_eCurAttackType == AttackType::FableEX)
		{
			if (!m_bFXEffect_Active && m_pChannel_GrinderLeftFx == nullptr)
			{
				m_pChannel_GrinderLeftFx = m_pGameInstance->Play_Effect(
					"Effect_Repair",
					m_Pieces[PIECE_LEFT].pModel,
					"BN_Blade_B",
					&m_Pieces[PIECE_LEFT].CombinedWorld,
					_float3(0.f, 0.f, 0.f));

				m_pChannel_GrinderRightFx = m_pGameInstance->Play_Effect(
					"Effect_Repair",
					m_Pieces[PIECE_RIGHT].pModel,
					"BN_Blade_B",
					&m_Pieces[PIECE_RIGHT].CombinedWorld,
					_float3(0.f, 0.f, 0.f));

				m_bFXEffect_Active = true;
			}
		}
	}
	else if (m_bFXEffect_Active && m_pChannel_GrinderLeftFx)
	{
		m_pChannel_GrinderLeftFx->Stop();
		m_pChannel_GrinderLeftFx = nullptr;

		m_pChannel_GrinderRightFx->Stop();
		m_pChannel_GrinderRightFx = nullptr;
		m_bFXEffect_Active = false;
	}
}

void CScissor::Set_PieceTrail(PIECE_TYPE ePiece, _bool bActive)
{
	if (ePiece >= PIECE_TYPE::PIECE_END) return;

	WEAPON_PIECE& Piece = m_Pieces[ePiece];

	if (bActive && Piece.pTrailEffect == nullptr)
	{
		if (m_eCurAttackType == AttackType::End) return;
		if (m_iCurComboStep < 0 || m_iCurComboStep >= static_cast<_int>(MAX_COMBO)) return;

		string strFX;

		if (m_eCurAttackType == AttackType::Fable && m_iCurComboStep == 2 && ePiece == PIECE_MERGE)
			strFX = "Effect_Player_TwoHand_Pable_Trail_2";
		else strFX = m_FXNames[ETOUI(m_eCurAttackType)][m_iCurComboStep];

		if (strFX.empty() || Piece.iTrailTopIndex < 0 || Piece.iTrailBottomIndex < 0) return;

		const _float4x4* pTopBoneMat = Piece.pModel->Get_Bone_CombinedMatPtr(Piece.iTrailTopIndex);
		const _float4x4* pBottomBoneMat = Piece.pModel->Get_Bone_CombinedMatPtr(Piece.iTrailBottomIndex);
		if (pTopBoneMat == nullptr || pBottomBoneMat == nullptr) return;

		Piece.pTrailEffect = m_pGameInstance->Play_Effect(strFX, pTopBoneMat, pBottomBoneMat, &Piece.CombinedWorld);
	}
	else if (!bActive && Piece.pTrailEffect != nullptr)
	{
		Piece.pTrailEffect->Stop();
		Piece.pTrailEffect = nullptr;
	}
}

CScissor* CScissor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CScissor* pInstance = new CScissor(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CScissor");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CScissor::Clone(void* pArg)
{
	CScissor* pInstance = new CScissor(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CScissor");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CScissor::Free()
{
	__super::Free();
	if (m_pChannel_GrinderLeftFx) m_pChannel_GrinderLeftFx->Stop();
	if (m_pChannel_GrinderRightFx) m_pChannel_GrinderRightFx->Stop();
}

#include "UIPlayerHUDController.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "PlayerHUDViewModel.h"
#include "UIProgressBar.h"
#include "UIText.h"
#include "StatComponent.h"
#include "Equipment.h"
#include "Weapon.h"
#include "UIElement.h"
#include "Player.h"
#include "Inventory.h"
#include "UIImage.h"
#include "LegionArm_Manager.h"

CUIPlayerHUDController::CUIPlayerHUDController()
{
}

HRESULT CUIPlayerHUDController::Initialize(void* pArg)
{
 	PLAYERHUD_CONTROLLER_DESC* pDesc = static_cast<PLAYERHUD_CONTROLLER_DESC*>(pArg);

	if (pDesc == nullptr)
		return E_FAIL;

	if (pDesc->pStatCom == nullptr || pDesc->pEquipment == nullptr)
	{
		return E_FAIL;
	}

	m_pStatCom = pDesc->pStatCom;
	m_pEquipmentCom = pDesc->pEquipment;
	m_fHPDelaySpeed = pDesc->fHPDelaySpeed;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;


	m_pViewModel = CPlayerHUDViewModel::Create();
	if (m_pViewModel == nullptr)
		return E_FAIL;

	return S_OK;
}

HRESULT CUIPlayerHUDController::Bind_UI()
{
	if (FAILED(__super::Bind_UI()))
		return E_FAIL;

	m_pHPBar = static_cast<CUIProgressBar*>(Find_Element(m_strHPBarElementTag));
	m_pStaminaBar = static_cast<CUIProgressBar*>(Find_Element(m_strStaminaBarElementTag));

	for (_uint i = 0; i < 3; i++)
	{
		m_pFableBars[i] = static_cast<CUIProgressBar*>(Find_Element(m_strFableBarElementTags[i]));

		if (m_pFableBars[i] == nullptr)
			return E_FAIL;

		m_iFableFillSlotIDs[i] = m_pFableBars[i]->Find_SlotID(m_strFableFillSlotName);

		if (m_iFableFillSlotIDs[i] < 0)
			return E_FAIL;
	}

	if (m_pHPBar == nullptr || m_pStaminaBar == nullptr)
		return E_FAIL;
	
	m_iHPFillSlotID = m_pHPBar->Find_SlotID(m_strHPFillSlotName);
	m_iHPFXSlotID = m_pHPBar->Find_SlotID(m_strHPFXSlotName);
	m_iStaminaFillSlotID = m_pStaminaBar->Find_SlotID(m_strStaminaFillSlotName);

	if (m_iHPFillSlotID < 0 || m_iHPFXSlotID < 0 || m_iStaminaFillSlotID < 0)
		return E_FAIL;

	m_pErgoProfile = Find_Element(L"ErgoProfile");
	m_pErgoCountText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoCount");
	m_pErgoGetText = Find_ElementAs<CUIText>(L"ErgoGet");
	m_pHealCountText = Find_ElementAs<CUIText>(L"HealCount");

	m_pWeaponHUDRoots[0] = Find_Element(L"WeaponHUDPrimaryRoot");
	m_pWeaponHUDRoots[1] = Find_Element(L"WeaponHUDSecondaryRoot");

	m_pWeaponDurabilityBars[0] = Find_ElementAs<CUIProgressBar>(L"WeaponDurabilityPrimary");
	m_pWeaponDurabilityBars[1] = Find_ElementAs<CUIProgressBar>(L"WeaponDurabilitySecondary");

	m_pLegionArmImage = Find_ElementAs<CUIImage>(L"Arm");

	m_pItemSlotSelectedImages[0] = Find_ElementAs<CUIImage>(L"SelectSlot1");
	m_pItemSlotSelectedImages[1] = Find_ElementAs<CUIImage>(L"SelectSlot2");
	m_pItemSlotRKeyIcons[0] = Find_ElementAs<CUIImage>(L"Slot1_RIcon");
	m_pItemSlotRKeyIcons[1] = Find_ElementAs<CUIImage>(L"Slot2_RIcon");

	for (_uint iSlotIndex = 0; iSlotIndex < 2; ++iSlotIndex)
	{
		if (m_pItemSlotSelectedImages[iSlotIndex] == nullptr || m_pItemSlotRKeyIcons[iSlotIndex] == nullptr)
			return E_FAIL;

		// 플레이어 선택 상태를 처음 받기 전 한 프레임 동안 두 슬롯이 표시되는 것을 방지한다.
		m_pItemSlotSelectedImages[iSlotIndex]->Set_Visible(false);
		m_pItemSlotRKeyIcons[iSlotIndex]->Set_Visible(false);
	}

	if (m_pWeaponHUDRoots[0] == nullptr || m_pWeaponHUDRoots[1] == nullptr || m_pWeaponDurabilityBars[0] == nullptr || m_pWeaponDurabilityBars[1] == nullptr || m_pLegionArmImage == nullptr)
		return E_FAIL;

	if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr || m_pHealCountText == nullptr)
		return E_FAIL;

	for (_uint iSlotIndex = 0; iSlotIndex < 2; ++iSlotIndex)
	{
		m_iWeaponDurabilityFillSlotIDs[iSlotIndex] = m_pWeaponDurabilityBars[iSlotIndex]->Find_SlotID(L"Fill");

		if (m_iWeaponDurabilityFillSlotIDs[iSlotIndex] < 0)
			return E_FAIL;
	}

	m_pWeaponHUDRoots[0]->Set_Visible(true);
	m_pWeaponHUDRoots[1]->Set_Visible(false);

	m_pErgoGetText->Set_Text(L"");
	m_pErgoGetText->Set_Visible(false);

	// 최초 HUD 생성 시 Open 애니메이션을 재생합니다.
	if (m_pUIObject != nullptr && !m_pUIObject->Is_RenderSuppressed() && m_pUIObject->Has_Animation("Open"))
		m_pUIObject->Play_Animation("Open", true);

	CUIText* pComponentErgoGetText = Find_ElementAs<CUIText>(L"ErgoProfile.ErgoGet");

	if (pComponentErgoGetText != nullptr)
	{
		pComponentErgoGetText->Set_Text(L"");
		pComponentErgoGetText->Set_Visible(false);
	}

	return S_OK;
}

void CUIPlayerHUDController::Update_ViewModel(_float fTimeDelta)
{
	if (m_pStatCom == nullptr || m_pViewModel == nullptr)
		return;

	// 에르고 획득량 표시 시간을 매 프레임 감소시킨다.
	m_pViewModel->Update_ErgoGain(fTimeDelta);

	if (m_pInventory == nullptr || m_pLegionArmManager == nullptr)
		Bind_PlayerRuntime();
	
	if (m_pPlayer != nullptr)
		m_pViewModel->Set_SelectedItemSlot(m_pPlayer->Get_ItemSlot());

	if (m_pInventory != nullptr)
	{
		const _uint iCurrentErgo = m_pInventory->Get_Ergo();

		// 최초 연결 시 보유 에르고 전체가 획득량으로 표시되는 것을 방지한다.
		if (!m_bErgoInitialized)
		{
			m_iPreviousErgo = iCurrentErgo;
			m_bErgoInitialized = true;
		}
		else
		{
			// 에르고가 증가했을 때만 획득량으로 표시한다.
			if (iCurrentErgo > m_iPreviousErgo)
			{
				m_pViewModel->Show_ErgoGain(iCurrentErgo - m_iPreviousErgo);

				// 새로 획득하면 누적 수치를 표시하고 등장 애니메이션을 처음부터 재생한다.
				m_bErgoGetFadePlayed = false;
				m_pErgoGetText->Set_Visible(true);

				m_pGameInstance->Play_Sound("SE_PC_FX_Item_ErgoDrop_Off", SOUND_SFX, 0.5f, false, false);

				if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("GetErgo"))
					m_pUIObject->Play_Animation("GetErgo", true);
			}

			m_iPreviousErgo = iCurrentErgo;
		}

		m_pViewModel->Set_CurrentErgo(iCurrentErgo);
	}

	// 표시 종료 0.25초 전부터 퇴장 알파 애니메이션을 한 번만 재생한다.
	if (m_pViewModel->Is_ErgoGainVisible() && !m_bErgoGetFadePlayed && m_pViewModel->Get_ErgoGainRemain() <= 0.25f)
	{
		m_bErgoGetFadePlayed = true;

		if (m_pUIObject != nullptr && m_pUIObject->Has_Animation("GetErgoFade"))
			m_pUIObject->Play_Animation("GetErgoFade", true);
	}

	const _float fHPRatio = m_pStatCom->Get_HPPercent();
	const _float fStaminaRatio = m_pStatCom->Get_StaminaPercent();

	m_pViewModel->Set_HPRatio(fHPRatio);
	m_pViewModel->Set_StaminaRatio(fStaminaRatio);
	m_pViewModel->Set_HealCount(m_pStatCom->Get_PotionCount());

	_float fDelayRatio = m_pViewModel->Get_HPDelayRatio();

	if (fHPRatio < fDelayRatio)
	{
		fDelayRatio = Move_Toward(fDelayRatio, fHPRatio, m_fHPDelaySpeed * fTimeDelta);

		m_pViewModel->Set_HPDelayRatio(fDelayRatio);
		m_pViewModel->Set_HPFXVisible(true);
	}
	else
	{
		m_pViewModel->Set_HPDelayRatio(fHPRatio);
		m_pViewModel->Set_HPFXVisible(false);
	}

	const _float fFableValue = Calc_Ratio(m_pStatCom->Get_CurrentFableStamina(), m_pStatCom->Get_MaxFableStamina()) * 3.f;
	m_pViewModel->Set_FableValue(fFableValue);

	
	_int iActiveWeaponSlot = -1;
	_float fDurabilityRatio = 0.f;

	if (m_pEquipmentCom != nullptr)
	{
		CWeapon* pCurrentWeapon = m_pEquipmentCom->Get_CurrentWeapon();

		if (pCurrentWeapon != nullptr)
		{
			const _uint64 iCurrentWeaponID = pCurrentWeapon->Get_ItemID();

			// 현재 무기 ID를 장착 슬롯의 아이템 ID와 비교해 활성 슬롯을 판별한다.
			if (m_pEquipmentCom->Get_EquippedItemID(0) == iCurrentWeaponID)
				iActiveWeaponSlot = 0;
			else if (m_pEquipmentCom->Get_EquippedItemID(1) == iCurrentWeaponID)
				iActiveWeaponSlot = 1;

			fDurabilityRatio = Calc_Ratio(pCurrentWeapon->Get_Durability(), 100.f);
		}
	}

	m_pViewModel->Set_ActiveWeaponSlot(iActiveWeaponSlot);
	m_pViewModel->Set_DurabilityRatio(fDurabilityRatio);

	if (m_pLegionArmManager != nullptr)
		m_pViewModel->Set_LegionArmType(m_pLegionArmManager->Get_CurrentArmType());
}

void CUIPlayerHUDController::Apply_ViewModel()
{
	if (m_pViewModel == nullptr || m_pViewModel->Is_Dirty() == false)
		return;

	if (m_pHPBar != nullptr)
	{
		m_pHPBar->Set_SlotRange(m_iHPFillSlotID, 0.f, m_pViewModel->Get_HPDelayRatio());
		m_pHPBar->Set_SlotVisible(m_iHPFXSlotID, m_pViewModel->Is_HPFXVisible());
	}

	if (m_pStaminaBar != nullptr)
	{
		m_pStaminaBar->Set_SlotRange(m_iStaminaFillSlotID, 0.f, m_pViewModel->Get_StaminaRatio());
	}

	for (_uint i = 0; i < 3; i++)
	{
		if (m_pFableBars[i] == nullptr)
			continue;

		m_pFableBars[i]->Set_SlotRange(m_iFableFillSlotIDs[i], 0.f, m_pViewModel->Get_FableSlotRatio(i));
	}

	Apply_WeaponHUD();
	Apply_LegionArmHUD();
	Apply_ItemSlotHUD();
	Apply_ErgoProfile();

	if (m_pHealCountText != nullptr)
		m_pHealCountText->Set_Text(to_wstring(m_pViewModel->Get_HealCount()));

	m_pViewModel->Clear_Dirty();
}

_float CUIPlayerHUDController::Calc_Ratio(_float fCurrent, _float fMax) const
{
	if (fMax <= 0.f)
		return 0.f;

	return min(1.f, max(0.f, fCurrent / fMax));
}

_float CUIPlayerHUDController::Move_Toward(_float fCurrent, _float fTarget, _float fStep) const
{
	if (fCurrent < fTarget)
		return min(fCurrent + fStep, fTarget);

	if (fCurrent > fTarget)
		return max(fCurrent - fStep, fTarget);

	return fTarget;
}

HRESULT CUIPlayerHUDController::Bind_PlayerRuntime()
{
	m_pPlayer = dynamic_cast<CPlayer*>(m_pGameInstance->Get_Player());

	if (m_pPlayer == nullptr)
		return E_FAIL;

	m_pInventory = m_pPlayer->Get_Inventory();
	m_pLegionArmManager = m_pPlayer->Get_LegionArmManager();

	if (m_pInventory == nullptr || m_pLegionArmManager == nullptr)
		return E_FAIL;

	return S_OK;
}

void CUIPlayerHUDController::Apply_ErgoProfile()
{
	if (m_pErgoProfile == nullptr || m_pErgoCountText == nullptr || m_pErgoGetText == nullptr)
		return;

	m_pErgoProfile->Set_Visible(true);
	m_pErgoCountText->Set_Text(Format_Number(m_pViewModel->Get_CurrentErgo()));

	const _bool bShowErgoGain = m_pViewModel->Is_ErgoGainVisible();

	m_pErgoGetText->Set_Visible(bShowErgoGain);

	if (bShowErgoGain)
		m_pErgoGetText->Set_Text(L"+" + Format_Number(m_pViewModel->Get_ErgoGain()));
	else
		m_pErgoGetText->Set_Text(L"");
}

void CUIPlayerHUDController::Apply_WeaponHUD()
{
	const _int iActiveWeaponSlot = m_pViewModel->Get_ActiveWeaponSlot();

	for (_uint iSlotIndex = 0; iSlotIndex < 2; ++iSlotIndex)
	{
		const _bool bActive = iActiveWeaponSlot == static_cast<_int>(iSlotIndex);

		m_pWeaponHUDRoots[iSlotIndex]->Set_Visible(bActive);

		if (bActive)
		{
			m_pWeaponDurabilityBars[iSlotIndex]->Set_SlotRange(m_iWeaponDurabilityFillSlotIDs[iSlotIndex], 0.f, m_pViewModel->Get_RepairGaugeRatio());
		}
	}
}

void CUIPlayerHUDController::Apply_LegionArmHUD()
{
	const LegionArmType eCurrentType = m_pViewModel->Get_LegionArmType();

	if (m_eAppliedLegionArmType == eCurrentType)
		return;

	const string strTexturePath = Resolve_LegionArmIconPath(eCurrentType);

	if (!strTexturePath.empty())
		Set_ImageTexture(L"Arm", strTexturePath);

	m_eAppliedLegionArmType = eCurrentType;
}

string CUIPlayerHUDController::Resolve_LegionArmIconPath(LegionArmType eType) const
{
	switch (eType)
	{
	case LegionArmType::ProtoArm:
		return "UI/Item/Arm/UIT_Item_Arm_1.dds";

	case LegionArmType::Grapple:
		return "UI/Item/Arm/UIT_Item_Arm_GG_01.dds";

	case LegionArmType::ChargeGun:
		return "UI/Item/Arm/UIT_Item_Arm_SniperCannon.dds";

	default:
		return "";
	}
}

HRESULT CUIPlayerHUDController::Set_ImageTexture(const _wstring& strImageTag, const string& strTexturePath)
{
	CUIImage* pImage = Find_ElementAs<CUIImage>(strImageTag);

	if (pImage == nullptr)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Set_UIImageTexture(m_hUI, strImageTag, strTexturePath)))
		return E_FAIL;

	pImage->Set_UseTexture(true);
	pImage->Set_Visible(true);

	return S_OK;
}

void CUIPlayerHUDController::Apply_ItemSlotHUD()
{
	const ITEM_SLOT eSelectedItemSlot = m_pViewModel->Get_SelectedItemSlot();

	// 화면의 1번 슬롯은 회복 아이템, 2번 슬롯은 그라인더다.
	const _bool bHealSelected = eSelectedItemSlot == ITEM_SLOT::Heal;
	const _bool bGrinderSelected = eSelectedItemSlot == ITEM_SLOT::Grinder;

	m_pItemSlotSelectedImages[0]->Set_Visible(bHealSelected);
	m_pItemSlotRKeyIcons[0]->Set_Visible(bHealSelected);

	m_pItemSlotSelectedImages[1]->Set_Visible(bGrinderSelected);
	m_pItemSlotRKeyIcons[1]->Set_Visible(bGrinderSelected);
}

_wstring CUIPlayerHUDController::Format_Number(_uint iValue) const
{
	_wstring strValue = to_wstring(iValue);

	// 숫자 뒤에서부터 세 자리마다 쉼표를 삽입한다.
	for (_int iPosition = static_cast<_int>(strValue.length()) - 3; iPosition > 0; iPosition -= 3)
		strValue.insert(static_cast<size_t>(iPosition), 1, L',');

	return strValue;
}

CUIPlayerHUDController* CUIPlayerHUDController::Create()
{
	return new CUIPlayerHUDController;
}

void CUIPlayerHUDController::Free()
{
	__super::Free();

	Safe_Release(m_pViewModel);
	
	m_pPlayer = nullptr;
	m_pLegionArmManager = nullptr;
	m_pLegionArmImage = nullptr;

	for (_uint iSlotIndex = 0; iSlotIndex < 2; ++iSlotIndex)
	{
		m_pItemSlotSelectedImages[iSlotIndex] = nullptr;
		m_pItemSlotRKeyIcons[iSlotIndex] = nullptr;
	}

	for (_uint iSlotIndex = 0; iSlotIndex < 2; ++iSlotIndex)
	{
		m_pWeaponHUDRoots[iSlotIndex] = nullptr;
		m_pWeaponDurabilityBars[iSlotIndex] = nullptr;
	}

	m_pStatCom = nullptr;
	m_pEquipmentCom = nullptr;

	m_pTestText = nullptr;
	m_pHPBar = nullptr;
	m_pStaminaBar = nullptr;

	m_pInventory = nullptr;
	m_pErgoProfile = nullptr;
	m_pErgoCountText = nullptr;
	m_pErgoGetText = nullptr;
	m_pHealCountText = nullptr;

	for (_uint i = 0; i < 3; i++)
	{
		m_pFableBars[i] = nullptr;
	}
}

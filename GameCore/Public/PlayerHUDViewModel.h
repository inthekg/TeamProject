#pragma once
#include "UIHUDViewModelBase.h"

NS_BEGIN(GameCore)

class CPlayerHUDViewModel final : public CUIHUDViewModelBase
{
private:
	CPlayerHUDViewModel() = default;
	virtual ~CPlayerHUDViewModel() = default;

public:
	void	Set_HPDelayRatio(_float fRatio);
	void	Set_StaminaRatio(_float fRatio);
	void	Set_FableValue(_float fValue);
	void	Set_DurabilityRatio(_float fRatio);

	void	Set_HPFXVisible(_bool bVisible);

	void	Set_CurrentErgo(_uint iCurrentErgo);

	void	Set_ActiveWeaponSlot(_int iSlotIndex);
	void	Set_LegionArmType(LegionArmType eType);
	void	Show_ErgoGain(_uint iGain);
	void	Update_ErgoGain(_float fTimeDelta);
	void	Set_SelectedItemSlot(ITEM_SLOT eItemSlot);

	void	Set_HealCount(_uint iHealCount);

public:
	_float	Get_HPDelayRatio() const { return m_fHPDelayRatio; }
	_float	Get_StaminaRatio() const { return m_fStaminaRatio; }
	_float	Get_FableSlotRatio(_uint iIndex) const;
	_float	Get_RepairGaugeRatio() const { return m_fDurabilityRatio; }

	_bool	Is_HPFXVisible() const { return m_bHPFXVisible; }

	_uint	Get_CurrentErgo() const { return m_iCurrentErgo; }

	_int	Get_ActiveWeaponSlot() const { return m_iActiveWeaponSlot; }
	LegionArmType Get_LegionArmType() const { return m_eLegionArmType; }
	_uint	Get_ErgoGain() const { return m_iErgoGain; }
	_bool	Is_ErgoGainVisible() const { return m_fErgoGainRemain > 0.f; }
	_float	Get_ErgoGainRemain() const { return m_fErgoGainRemain; }
	ITEM_SLOT Get_SelectedItemSlot() const { return m_eSelectedItemSlot; }

	_uint	Get_HealCount() const { return m_iHealCount; }

private:
	_float	m_fHPDelayRatio = { 1.f };		// 플레이어 지연 체력 비율.

	_float	m_fStaminaRatio = { 1.f };

	_bool	m_bHPFXVisible = { true };

	_float	m_fFableValue = {};
	_float	m_fFableSlotRatios[3] = { 0.f, 0.f, 0.f };

	_float	m_fDurabilityRatio = 1.f;

	_uint	m_iCurrentErgo = { 0 };

	_int	m_iActiveWeaponSlot = { -1 }; // 현재 손에 들고 있는 무기 슬롯이다.
	LegionArmType m_eLegionArmType = { LegionArmType::End }; // 현재 장착 중인 리전 암 종류다.

	_uint	m_iErgoGain = { 0 }; // 화면에 표시할 누적 에르고 획득량이다.
	_float	m_fErgoGainRemain = { 0.f }; // 에르고 획득량 표시 남은 시간이다.

	ITEM_SLOT m_eSelectedItemSlot = { ITEM_SLOT::End }; // 플레이어가 현재 선택한 소비 아이템 슬롯

	_uint	m_iHealCount = { 0 }; // 현재 플레이어가 보유한 회복 아이템 개수

public:
	static CPlayerHUDViewModel* Create();
	virtual void Free() override;
};

NS_END
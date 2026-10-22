#pragma once
#include "UIControllerBase.h"

NS_BEGIN(Engine)
class CUIProgressBar;
class CUIText;
class CUIElement;
class CUIImage;
NS_END

NS_BEGIN(GameCore)

class CPlayerHUDViewModel;
class CStatComponent;
class CEquipment;
class CInventory;
class CPlayer;
class CLegionArm_Manager;

class CUIPlayerHUDController final : public CUIControllerBase
{
public:
	typedef struct tagPlayerHUDControllerDesc : public CUIControllerBase::UI_CONTROLLER_DESC
	{
		CStatComponent* pStatCom = { nullptr };
		CEquipment* pEquipment = { nullptr };

		_float fHPDelaySpeed = { 0.8f };
	}PLAYERHUD_CONTROLLER_DESC;

private:
	CUIPlayerHUDController();
	virtual ~CUIPlayerHUDController() = default;
	
public:
	virtual HRESULT			Initialize(void* pArg) override;

protected:
	virtual HRESULT			Bind_UI() override;
	virtual void			Update_ViewModel(_float fTimeDelta) override;
	virtual void			Apply_ViewModel() override;

private:
	_float					Calc_Ratio(_float fCurrent, _float fMax) const;
	_float					Move_Toward(_float fCurrent, _float fTarget, _float fStep) const;

	HRESULT					Bind_PlayerRuntime();
	_wstring				Format_Number(_uint iValue) const;
	void					Apply_ErgoProfile();

	void					Apply_WeaponHUD();
	void					Apply_LegionArmHUD();
	string					Resolve_LegionArmIconPath(LegionArmType eType) const;
	HRESULT					Set_ImageTexture(const _wstring& strImageTag, const string& strTexturePath);

	void					Apply_ItemSlotHUD();

private:
	CPlayer*				m_pPlayer = { nullptr };
	CLegionArm_Manager*		m_pLegionArmManager = { nullptr };
	CStatComponent*			m_pStatCom = { nullptr };
	CEquipment*				m_pEquipmentCom = { nullptr };
	CPlayerHUDViewModel*	m_pViewModel = { nullptr };
	CInventory*				m_pInventory = { nullptr };


private:
	CUIElement*				m_pWeaponHUDRoots[2] = {};
	CUIProgressBar*			m_pWeaponDurabilityBars[2] = {};

	CUIImage*				m_pLegionArmImage = { nullptr };
	LegionArmType			m_eAppliedLegionArmType = { LegionArmType::End };

	CUIProgressBar*			m_pHPBar = { nullptr };
	CUIProgressBar*			m_pStaminaBar = { nullptr };
	CUIProgressBar*			m_pFableBars[3] = {};

	CUIImage*				m_pItemSlotSelectedImages[2] = {}; // 각 아이템 슬롯의 빨간 선택 배경
	CUIImage*				m_pItemSlotRKeyIcons[2] = {};      // 선택된 슬롯에만 표시할 R키 아이콘

	CUIElement*				m_pErgoProfile = { nullptr };
	CUIText*				m_pErgoCountText = { nullptr };
	CUIText*				m_pErgoGetText = { nullptr };
	CUIText*				m_pHealCountText = { nullptr }; // 현재 회복 아이템 개수 텍스트

	CUIText*				m_pTestText = { nullptr };

	_float					m_fHPDelaySpeed = { 0.8f };

	_wstring                m_strHPBarElementTag = L"HPBar";
	_wstring                m_strHPFillSlotName = L"Fill";
	_wstring                m_strHPFXSlotName = L"FX";

	_wstring                m_strStaminaBarElementTag = L"Stamina";
	_wstring                m_strStaminaFillSlotName = L"Fill";

	_wstring				m_strFableBarElementTags[3] = { L"Fable1", L"Fable2", L"Fable3" };
	_wstring				m_strFableFillSlotName = L"Fill";
	_wstring				m_strTestTextElementTag = L"TestText";

	_int                    m_iHPFillSlotID = -1;
	_int                    m_iHPFXSlotID = -1;
	_int                    m_iStaminaFillSlotID = -1;
	_int					m_iFableFillSlotIDs[3] = { -1, -1, -1 };

	_int					m_iWeaponDurabilityFillSlotIDs[2] = { -1, -1 };

	_uint					m_iPreviousErgo = { 0 };
	_bool					m_bErgoInitialized = { false };

	_bool					m_bErgoGetFadePlayed = { false };

public:
	static CUIPlayerHUDController* Create();
	virtual void Free() override;
};

NS_END
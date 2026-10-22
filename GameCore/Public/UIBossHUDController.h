#pragma once  
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CUIObject;
class CUIProgressBar;
class CUIElement;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CAIObject;
class CBossHUDViewModel;
class CStatComponent_AI;

class CUIBossHUDController final : public CUIControllerBase, public IListener
{
private:
	enum class BOSS_HUD_ANIM_STATE
	{
		NONE,
		GROGGY,
		GROGGY_SUCCESS,
		FATAL_ACTIVE,
		FATAL_POSSIBLE
	};

public:
	typedef struct tagBossHUDControllerDesc : public CUIControllerBase::UI_CONTROLLER_DESC
	{
		// 보스를 찾을 게임 레이어
		_wstring strBossLayerTag = TEXT("Layer_Monster");

		// Fatal 위치 보정 값
		_float2 vFatalScreenOffset = { 0.f, 0.f };
	}BOSSHUD_CONTROLLER_DESC;

private:
	CUIBossHUDController();
	virtual ~CUIBossHUDController() = default;

public:
	virtual HRESULT		Initialize(void* pArg) override;

protected:
	virtual HRESULT		Bind_UI() override;
	virtual void		Update_ViewModel(_float fTimeDelta) override;
	virtual void		Apply_ViewModel() override;

public:
	virtual void		OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;

private:
	void				Change_HUDAnimation(BOSS_HUD_ANIM_STATE eNextState);
	void				Reset_HUDAnimation();
	HRESULT				Bind_Boss(Event_Desc* pData);

private:
	_wstring			m_strHPBarElementTag = { L"BossHPBar" };
	_wstring			m_strFillSlotName = { L"Fill" };
	_wstring			m_strBossLayerTag = TEXT("Layer_Monster");

 	CAIObject*			m_pBoss = { nullptr };
	CStatComponent_AI*	m_pBossStatCom = { nullptr };
	CUIProgressBar*		m_pHPBar = { nullptr };
	CBossHUDViewModel*	m_pViewModel = { nullptr };

	_int				m_iFillSlotID = { -1 };
	_bool				m_bBossStarted = { false };

	_wstring			m_strGroggyElementTag = { L"BossGroggy" };
	CUIElement*			m_pGroggy = { nullptr };

	_wstring			m_strFatalElementTag = { L"BossFatal" };
	_float2				m_vFatalScreenOffset = { _float2(0.f, 0.f) };
	CUIElement*			m_pFatal = { nullptr };

	_wstring			m_strBossName = {};

	_wstring            m_strDamageTextElementTag = { L"DamageText" };
	CUIText*			m_pDamageText = { nullptr };


	_wstring			m_strBossNameElementTag = { L"BossName" };
	CUIText*			m_pBossNameText = { nullptr };

	_wstring			m_strGroggyElementTag2 = { L"BossGroggy2" };
	_wstring			m_strGroggyElementTag3 = { L"BossGroggy3" };
	CUIElement*			m_pGroggy2 = { nullptr };
	CUIElement*			m_pGroggy3 = { nullptr };

	_float4				m_vFatalAvailableColor = {};
	_float4				m_vFatalUnavailableColor = { 0.9f, 0.08f, 0.08f, 1.f };

	BOSS_HUD_ANIM_STATE m_eHUDAnimState = BOSS_HUD_ANIM_STATE::NONE;
	_bool				m_bWasGroggy = false;

public:
	static CUIBossHUDController* Create();
	virtual void Free() override;
};

NS_END
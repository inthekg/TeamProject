#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CUIPlayerDeadController final : public CUIControllerBase, public IListener
{
private:
	CUIPlayerDeadController() = default;
	virtual ~CUIPlayerDeadController() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
	virtual void Update_ViewModel(_float fTimeDelta) override;
	virtual void Apply_ViewModel() override;

private:
	void Begin_PlayerDeadAnimation();		// 사망 UI를 열고 PlayerDead 애니메이션을 처음부터 재생한다.
	void Finish_PlayerDeadAnimation();		// 사망 UI 종료 후 기존 리스폰 이벤트를 발송한다.
	void Request_RespawnFallback();			// UI 재생 실패 시 게임이 멈추지 않도록 즉시 리스폰을 요청한다.

private:
	_bool m_bPlaying = { false };
	_bool m_bHUDSuppressed = { false };

public:
	static CUIPlayerDeadController* Create();
	virtual void Free() override;
};

NS_END
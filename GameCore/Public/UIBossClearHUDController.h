#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CUIBossClearHUDController final : public CUIControllerBase, public IListener
{
private:
	CUIBossClearHUDController() = default;
	virtual ~CUIBossClearHUDController() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

protected:
	virtual void Update_ViewModel(_float fTimeDelta) override;
	virtual void Apply_ViewModel() override;

private:
	void Begin_BossClearAnimation(); // UI를 열고 보스 클리어 애니메이션을 처음부터 재생한다.
	void Finish_BossClearAnimation(); // 애니메이션 종료 후 UI를 다시 닫는다.

private:
	_bool m_bPlaying = { false };

public:
	static CUIBossClearHUDController* Create();
	virtual void Free() override;
};

NS_END
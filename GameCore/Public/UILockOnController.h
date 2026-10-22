#pragma once
#include "UIControllerBase.h"

NS_BEGIN(Engine)
class CUIElement;
class CGameObject;
NS_END


NS_BEGIN(GameCore)

class CAIObject;
class CStatusComponent;
class CLockOnViewModel;

class CUILockOnController final : public CUIControllerBase
{
public:
	typedef struct tagLockOnControllerDesc : public CUIControllerBase::UI_CONTROLLER_DESC
	{
		CStatusComponent* pStatusCom = { nullptr };

		_float2 vScreenOffset = { 0.f, 0.f };
	}LOCKON_CONTROLLER_DESC;

private:
	CUILockOnController();
	virtual ~CUILockOnController() = default;

public:
	virtual HRESULT		Initialize(void* pArg) override;

protected:
	virtual HRESULT		Bind_UI() override;
	virtual void		Update_ViewModel(_float fTimeDelta) override;
	virtual void		Apply_ViewModel() override;

private:
	CGameObject*		m_pPreviousLockedTarget = { nullptr };	// 직전에 락온했던 타깃. 타깃 변경 감지용이며 소유하지 않는다.
	_bool				m_bPendingActiveAnimation = { false };	// UI 적용 단계에서 Active 애니메이션을 재생할지 여부

private:
	CStatusComponent*	m_pStatusCom = { nullptr };
	CLockOnViewModel*	m_pViewModel = { nullptr };

	CUIElement*			m_pLockOnElement = { nullptr };

	_wstring			m_strLockOnElementTag = L"LockOn";
	_float2				m_vScreenOffset = { 0.f, 0.f };

public:
	static CUILockOnController* Create();
	virtual void Free() override;
};

NS_END
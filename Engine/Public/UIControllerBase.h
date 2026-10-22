#pragma once
#include "Base.h"
#include "UIElement.h"
#include "UI_RuntimeData.h"

NS_BEGIN(Engine)

class CUIObject;
class CGameInstance;

class ENGINE_DLL CUIControllerBase abstract : public CBase
{
public:
	typedef struct tagUIControllerDesc
	{
		UI_HANDLE hUI = {};
	}UI_CONTROLLER_DESC;

protected:
	CUIControllerBase();
	virtual ~CUIControllerBase() = default;

public:
	virtual HRESULT		Initialize(void* pArg);
	virtual void		Update(_float fTimeDelta);

public:
	// 자신이 연결된 UI에서 발생한 Button Command 처리
	virtual void		On_UICommand(const UI_COMMAND_EVENT& Event);

	// 자신이 연결된 UI에서 Button Focus가 변경됐을 때 호출
	virtual void		On_UIFocusChanged(const UI_FOCUS_EVENT& Event);

protected:
	virtual HRESULT		Bind_UI();
	virtual void		Update_ViewModel(_float fTimeDelta) = 0;
	virtual void		Apply_ViewModel() = 0;

protected:
	CUIElement*			Find_Element(const _wstring& strElementTag);

	template<typename T>
	// BindingKey로 Element를 찾고 요청한 실제 타입인지 검사
	T* Find_ElementAs(const _wstring& strElementTag)
	{
		CUIElement* pElement = Find_Element(strElementTag);

		if (pElement == nullptr)
			return nullptr;

		return dynamic_cast<T*>(pElement);
	}

	_bool				WorldToScreen(_fvector vWorldPosition, _float2& vOutScreenPosition);

protected:
	CGameInstance*		m_pGameInstance = { nullptr };

	UI_HANDLE			m_hUI = {};
	CUIObject*			m_pUIObject = { nullptr };

public:
	virtual void Free() override;
};

NS_END
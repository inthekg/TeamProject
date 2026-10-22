#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLayer final : public CBase
{
private:
	CLayer();
	virtual ~CLayer() = default;

public:
	list<class CGameObject*>&			Get_Objects() { return m_GameObjects; }

	class CComponent*					Get_Component(const _wstring& strComponentTag, _uint iIndex);
	class CComponent*					Get_Component(const _wstring& strPartTag, const _wstring& strComponentTag, _uint iIndex);

public:
	HRESULT								Add_GameObject(class CGameObject* pGameObject);

	void								Priority_Update(_float fTimeDelta);
	void								Update(_float fTimeDelta);
	void                                Physical_Update(_float fFixedTime);
	void								Late_Update(_float fTimeDelta);

private:
	list<class CGameObject*>			m_GameObjects;
	//삭제될 오브젝트들. 1프레임 대기 후 삭제하게 함 
	list<class CGameObject*>		    m_PendingKillObjects;

public:
	static CLayer*						Create();
	virtual void						Free() override;
};

NS_END
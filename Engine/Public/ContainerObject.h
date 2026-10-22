#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CContainerObject abstract : public CGameObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CContainerObject)
#endif // _DEBUG
protected:
	CContainerObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CContainerObject(const CContainerObject& Prototype);
	virtual ~CContainerObject() = default;

public:
	virtual HRESULT									Initialize_Prototype() override;
	virtual HRESULT									Initialize(void* pArg) override;
	virtual void									Priority_Update(_float fTimeDelta) override;
	virtual void									Update(_float fTimeDelta) override;
	virtual void									Physical_Update(_float fFixedTime) override;
	virtual void									Late_Update(_float fTimeDelta) override;
	virtual HRESULT									Render() override;

public:
	//부모의 FindComponent를 자식 Scope로 명시적으로 가져와 오버로딩이 성립하게함 
	using CGameObject::Find_Component;

	class CComponent*								Find_Component(const _wstring& strPartTag, const _wstring& strComponentTag);
	CGameObject*									Get_PartObject(const _wstring& strPartTag);

#ifdef _DEBUG
	const map<const _wstring, class CPartObject*>& Get_PartObjects() const { return m_PartObjects; }
#endif // _DEBUG

protected:
	HRESULT											Add_PartObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, const _wstring& strPartTag, void* pArg = nullptr);

protected:
	map<const _wstring, class CPartObject*>			m_PartObjects;

public:
	// Create
	virtual CGameObject* Clone(void* pArg) = 0;
	virtual void Free();

};

NS_END
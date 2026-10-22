#pragma once
#include "StaticObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCustomStaticObject : public CStaticObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CCustomStaticObject)
#endif
protected:
	CCustomStaticObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContexts);
	CCustomStaticObject(const CCustomStaticObject& Prototype);
	virtual ~CCustomStaticObject() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;

	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	HRESULT					Set_Model(CModel* pModelCom);

#ifdef _DEBUG
	HRESULT					Set_ModelByName(_wstring strModelName);

protected :
	_wstring				m_strShaderName = {};
	_wstring				m_strModelName = {};
#endif // _DEBUG


public:
	static CCustomStaticObject*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg);
	virtual void			Free();
};

NS_END
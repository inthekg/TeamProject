#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CMaterial;
class CModel;

class ENGINE_DLL CStaticObject : public CGameObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CStaticObject)
#endif
protected:
	CStaticObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContexts);
	CStaticObject(const CStaticObject& Prototype);
	virtual ~CStaticObject() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;

	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override; 

protected:
	virtual HRESULT			Ready_Components();

protected:
	CModel*					m_pModelCom		= { nullptr };

public:
	static CStaticObject*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg);
	virtual void			Free();
};

NS_END
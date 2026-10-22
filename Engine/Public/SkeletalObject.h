#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;

class ENGINE_DLL CSkeletalObject abstract : public CGameObject
{
protected:
	CSkeletalObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSkeletalObject(const CSkeletalObject& Prototype);
	virtual ~CSkeletalObject() = default;

public:
	virtual HRESULT         Initialize_Prototype() override;
	virtual HRESULT         Initialize(void* pArg) override;

	virtual void            Priority_Update(_float fTimeDelta) override;
	virtual void            Update(_float fTimeDelta) override;
	virtual void            Physical_Update(_float fFixedTime) override;
	virtual void            Late_Update(_float fTimeDelta) override;
	virtual HRESULT         Render();

public:
	CModel*					Get_Model() const { return m_pModelCom;}

protected:
	virtual HRESULT			Ready_Components();

protected:
	CModel*                 m_pModelCom = { nullptr };

public:
	virtual CGameObject*    Clone(void* pArg) = 0;
	virtual void            Free();
};

NS_END
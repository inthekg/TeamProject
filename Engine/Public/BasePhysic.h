#pragma once
#include "Component.h"

NS_BEGIN(Engine)

class CGameObject;
class CTransform;
class ENGINE_DLL CBasePhysic : public CComponent
{
protected:
	CBasePhysic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBasePhysic(const CBasePhysic& Prototype);
	virtual ~CBasePhysic() = default;

public:
	virtual HRESULT		Initialize_Prototype() override;
	virtual HRESULT		Initialize(void* pArg) override;

	virtual void		Update(_float fTimeDelta) {};
	virtual void		Physical_Update(_float fFixedTime) {};

public:
	HRESULT				Add_Shape(const SHAPE_DESC& ShapeDesc);
	void				Set_Filter(PxShape* pShape, _uint iWord0, _uint iWord1);
	
	CGameObject*		Get_Owner() const { return m_pOwner; }

protected:
	CGameObject*		m_pOwner = { nullptr };
	CTransform*			m_pOwnerTransformCom = { nullptr };

	vector<PxShape*>	m_PhysicShapes;
	_uint				m_iFilterWord0 = {};
	_uint				m_iFilterWord1 = {};

public:
	virtual void		Free() override;
};

NS_END

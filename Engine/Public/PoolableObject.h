#pragma once
#include "GameObject.h"
#include "IPoolable.h"

NS_BEGIN(Engine)

class ENGINE_DLL CPoolableObject : public CGameObject, public IPoolable
{
protected:
	CPoolableObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CPoolableObject() = default;

public:
	virtual _uint		Release() override;

public:
	virtual void		Activate(void* pArg = nullptr) override	{ Set_Active(); }
	virtual void		InActivate() override { Set_InActive(); }

public:
	virtual void		Free() override;

};

NS_END
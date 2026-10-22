#pragma once
#include "RigidActor.h"

NS_BEGIN(Engine)
/* 이동이 없는 충돌체. 한번 세팅한 뒤로는 변화없음*/
class ENGINE_DLL CStaticRigidBody : public CRigidActor
{
private:
	CStaticRigidBody(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStaticRigidBody(const CStaticRigidBody& Prototype);
	virtual ~CStaticRigidBody() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;

public:
	static CStaticRigidBody*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*			Clone(void* pArg) override;
	virtual void				Free() override;

};

NS_END
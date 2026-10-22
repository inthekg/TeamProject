#pragma once
#include "BasePhysic.h"

NS_BEGIN(Engine)
class ENGINE_DLL CRigidActor abstract : public CBasePhysic
{
protected:
	CRigidActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CRigidActor(const CRigidActor& Prototype);
	virtual ~CRigidActor() = default;

public:
	virtual HRESULT		Initialize_Prototype() override;
	virtual HRESULT		Initialize(void* pArg) override;
	virtual void		Physical_Update(_float fFixedTime) override;
	
public:
	HRESULT				Setup_RigidActor(PxRigidActor* pActor);

	PxRigidActor*		Get_RigidActor() const { return m_pRigidActor; }
	void				Get_GlobalTransform(OUT _vector& pOutPos, OUT _vector& pOutQuat);
	
	void				Set_GlobalTransform(_fvector vPos, _fvector vQuat);
	void				Set_ActorFlag(PxActorFlag::Enum eFlag, _bool bEnable);

protected:
	PxRigidActor*		m_pRigidActor = { nullptr };

public:
	virtual void		Free() override;

};

NS_END
#pragma once
#include "RigidActor.h"

NS_BEGIN(Engine)

/* 
// kinematic이 true -> false로 전환되는 경우는 제외. 해당 상황인 레그돌은 다른 컴포넌트로 구현함. 
// kinematic이 설정된 경우, Simulation전에 update, 아니면 후에 Update가 돌아야함 
//*/

class ENGINE_DLL CDynamicRigidBody :public CRigidActor
{
private:
	CDynamicRigidBody(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CDynamicRigidBody(const CDynamicRigidBody& Prototype);
	virtual ~CDynamicRigidBody() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;	

	virtual void				Physical_Update(_float fFixedTime) override;
public:
	void						Add_Force(_fvector vForce, PxForceMode::Enum eMode = PxForceMode::eFORCE);
	//회전력
	void						Add_Torque(_fvector vTorque, PxForceMode::Enum eMode = PxForceMode::eFORCE);

	_bool						Is_Kinematic() const;
	void						Set_CCD(_bool bEnable);
public:
	// 일반 kinematic Actor(본 없을때) 일때
	HRESULT						Set_KinematicTarget(_fvector vPosition, _fvector vQuaternion);

	//Kinematric true일때 
	void						Attach_ToBone(const _float4x4* pBoneCombinedMatrix, _fmatrix offsetMatrix);

private:
	//m_pRigidActor와 같은 포인터. Dynamic 기능을 사용하기위해 저장 
	PxRigidDynamic*				m_pRigidDynamic = { nullptr };	
	
	const _float4x4*			m_pBindBone_CombinedMatrix = { nullptr };
	_float4x4					m_OffsetMatrix = {};

public:
	static CDynamicRigidBody*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*			Clone(void* pArg);
	virtual void				Free() override;

	
};

NS_END

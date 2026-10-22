#include "DynamicRigidBody.h"
#include "GameInstance.h"
#include "Transform.h"

CDynamicRigidBody::CDynamicRigidBody(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CRigidActor{ pDevice, pContext }
{
}

CDynamicRigidBody::CDynamicRigidBody(const CDynamicRigidBody& Prototype)
	: CRigidActor{ Prototype }
{
}

HRESULT CDynamicRigidBody::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CDynamicRigidBody::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	PHYSIC_DESC* pPhysicDesc = reinterpret_cast<PHYSIC_DESC*>(pArg);
	
	if (FAILED(__super::Initialize(pPhysicDesc)))
		return E_FAIL;

	/* Actor 생성 */
	_vector vPos = XMLoadFloat3(&pPhysicDesc->vInitPos);
	_vector vQuat = XMLoadFloat4(&pPhysicDesc->vInitQuat);

	m_pRigidDynamic = CGameInstance::Get_Instance()->Create_RigidDynamic(vPos);
	if (m_pRigidDynamic == nullptr) return E_FAIL;


	if(FAILED(Setup_RigidActor(m_pRigidDynamic)))
	{
		MSG_BOX("DynamicRigidBody : Setup Physic Actor Failed");
		return E_FAIL;
	}	


	/* CreateDynamic은 초기 위치만 세팅. 회전 변환 */
	Set_GlobalTransform(vPos, vQuat);

	/* Set Kinematic */
	m_pRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, pPhysicDesc->bIsKinematic);

	//CCD 활성화 
	if(pPhysicDesc->bEnableCCD && !pPhysicDesc->bIsKinematic)
	{
		m_pRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
	}

	/* 관성 계산하는 유틸리티 함수. mass질량을 설정하고 Shape를 분석해서 무게중심 찾아서 세팅함. 단, Simulation용 Shape가 있어야함 */
	if(!pPhysicDesc->bIsKinematic)
		PxRigidBodyExt::setMassAndUpdateInertia(*m_pRigidDynamic, pPhysicDesc->fMass);

	return S_OK;
}

void CDynamicRigidBody::Physical_Update(_float fFixedTime)
{
	if (m_pRigidDynamic == nullptr) return;

	//kinematic인 경우 ( 이동변화량을 나 -> 강체에게 적용 )
	if (m_pBindBone_CombinedMatrix)
	{
		//본의 월드 변환에 오프셋 곱해서 최종 kinematic 타겟 변환 구하기 
		_matrix CombinedBoneMatrix = XMLoadFloat4x4(m_pBindBone_CombinedMatrix);
		_matrix OwnerWolrdMatrix = XMLoadFloat4x4(m_pOwnerTransformCom->Get_WorldMatrixPtr());
		_matrix TargetMatrix = XMLoadFloat4x4(&m_OffsetMatrix) * CombinedBoneMatrix * OwnerWolrdMatrix;

		_vector vScale, vQuat, vTrans;
		XMMatrixDecompose(&vScale, &vQuat, &vTrans, TargetMatrix);

		PxTransform transform;
		transform.p = PxVec3(XMVectorGetX(vTrans), XMVectorGetY(vTrans), XMVectorGetZ(vTrans));
		transform.q = PxQuat(XMVectorGetX(vQuat), XMVectorGetY(vQuat), XMVectorGetZ(vQuat), XMVectorGetW(vQuat));
		//해당 목적지를 kinematic의 타겟으로 세팅. 시뮬레이션에 의해 이동됨 
		m_pRigidDynamic->setKinematicTarget(transform);
		
		return;
	}

	//kinematic 이 아닌경우 ( 강체 -> 나 에게 적용 )
	if(!m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
	{
		//kinematic이 아니면 시뮬레이션에 의해 이동된 위치를 오너에게 전달함
		_vector vPos, vQuat;
		Get_GlobalTransform(vPos, vQuat);

		m_pOwnerTransformCom->Set_Quat(vQuat);
		m_pOwnerTransformCom->Set_State(STATE::POSITION, vPos);
	}

}

void CDynamicRigidBody::Add_Force(_fvector vForce, PxForceMode::Enum eMode)
{
	if (m_pRigidDynamic == nullptr) return;	
	//키네마틱이면패스
	if (m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;
	m_pRigidDynamic->addForce(PxVec3(XMVectorGetX(vForce), XMVectorGetY(vForce), XMVectorGetZ(vForce)), eMode);
}

void CDynamicRigidBody::Add_Torque(_fvector vTorque, PxForceMode::Enum eMode)
{
	if (m_pRigidDynamic == nullptr) return;
	//키네마틱이면패스
	if (m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;
	m_pRigidDynamic->addTorque(PxVec3(XMVectorGetX(vTorque), XMVectorGetY(vTorque), XMVectorGetZ(vTorque)), eMode);
}

_bool CDynamicRigidBody::Is_Kinematic() const
{
	if (m_pRigidDynamic == nullptr) return false;
	return m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
}

void CDynamicRigidBody::Set_CCD(_bool bEnable)
{
	if(m_pRigidDynamic == nullptr) return;

	//CCD는 kinematic과 호환되지 않음. kinematic이면 패스
	if (bEnable && m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;

	m_pRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, bEnable);
}

HRESULT CDynamicRigidBody::Set_KinematicTarget(_fvector vPosition, _fvector vQuaternion)
{
	if (nullptr == m_pRigidDynamic)
		return E_FAIL;

	if (false == m_pRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
		return E_FAIL;

	if (nullptr == m_pRigidDynamic->getScene())
		return E_FAIL;

	PxTransform Target{};
	Target.p = PxVec3(XMVectorGetX(vPosition), XMVectorGetY(vPosition), XMVectorGetZ(vPosition));
	Target.q = PxQuat(XMVectorGetX(vQuaternion), XMVectorGetY(vQuaternion), XMVectorGetZ(vQuaternion), XMVectorGetW(vQuaternion));

	m_pRigidDynamic->setKinematicTarget(Target);

	return S_OK;
}

//Kinematic
void CDynamicRigidBody::Attach_ToBone(const _float4x4* pBoneCombinedMatrix, _fmatrix offsetMatrix)
{
	if (!pBoneCombinedMatrix || !m_pRigidDynamic) return;

	m_pBindBone_CombinedMatrix = pBoneCombinedMatrix;
	XMStoreFloat4x4(&m_OffsetMatrix, offsetMatrix);

	m_pRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}

CDynamicRigidBody* CDynamicRigidBody::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CDynamicRigidBody* pInstance = new CDynamicRigidBody(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("DynamicRigidBody Create Failed");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CDynamicRigidBody::Clone(void* pArg)
{
	CDynamicRigidBody* pClone = new CDynamicRigidBody(*this);
	if (FAILED(pClone->Initialize(pArg)))
	{
		MSG_BOX("DynamicRigidBody Clone Failed");
		Safe_Release(pClone);
	}
	return pClone;
}

void CDynamicRigidBody::Free()
{
	__super::Free();
	m_pRigidDynamic = nullptr;  //rigidActor와 동일한 주소. Super에서 해제함 
}

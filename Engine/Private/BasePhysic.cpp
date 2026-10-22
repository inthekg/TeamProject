#include "BasePhysic.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Transform.h"

CBasePhysic::CBasePhysic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CComponent{ pDevice, pContext }
{
}

CBasePhysic::CBasePhysic(const CBasePhysic& Prototype)
	:CComponent{ Prototype }
{
}

HRESULT CBasePhysic::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CBasePhysic::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;

	PHYSIC_DESC* pPhysicDesc = reinterpret_cast<PHYSIC_DESC*>(pArg);
	if (pPhysicDesc == nullptr) return E_FAIL;

	m_pOwner = pPhysicDesc->pOwner;
	if (m_pOwner) m_pOwnerTransformCom = m_pOwner->Get_Transform();

	//내 필터링 정보
	m_iFilterWord0 = pPhysicDesc->iFilterWord0;
	//충돌 처리할 상대의 필터링 정보 
	m_iFilterWord1 = pPhysicDesc->iFilterWord1;


	for (SHAPE_DESC& ShapeDesc : pPhysicDesc->Shapes)
	{
		if(FAILED(Add_Shape(ShapeDesc)))
		{
			MSG_BOX("BasePhysic : Add Shape Failed");
			return E_FAIL;
		}
	}

    return S_OK;
}

HRESULT CBasePhysic::Add_Shape(const SHAPE_DESC& ShapeDesc)
{
	PxMaterial* pMaterial = CGameInstance::Get_Instance()->Get_Default_Material();
	if(pMaterial == nullptr)
	{
		MSG_BOX("BasePhysic : Get Default Material Failed");
		return E_FAIL;
	}

	//PxGeometryHolder : 모든 타입의 지오매트리를 Union으로 커버할수있는 클래스 
	PxGeometryHolder GeometryHolder;
	
	switch (ShapeDesc.eType)
	{
		case GEOMETRY_TYPE::BOX:
			GeometryHolder = PxBoxGeometry(ShapeDesc.vHalfExtents.x, ShapeDesc.vHalfExtents.y, ShapeDesc.vHalfExtents.z);
			break;

		case GEOMETRY_TYPE::SPHERE:
			GeometryHolder = PxSphereGeometry(ShapeDesc.fRadius);
			break;

		case GEOMETRY_TYPE::CAPSULE:
			GeometryHolder = PxCapsuleGeometry(ShapeDesc.fRadius, ShapeDesc.fHalfHeight);
			break;
		default: return E_FAIL;
	}

	//any : PX의 Any. 부모로 캐스팅해서 반환. CreatePxShape는 ref를 올리고 반환함. Attached할때도 addref. 때문에 Attached하면 release해주기 (온전히 해당 Actor에만 종속시킴)
	PxShape* pShape = m_pGameInstance->Create_PxShape(GeometryHolder.any(), pMaterial, true);
	if (pShape == nullptr)
	{
		MSG_BOX("BasePhysic : Create Shape Failed");
		return E_FAIL;
	}

	PxTransform LocalPose = PxTransform(PxVec3(ShapeDesc.vLocalOffset.x, ShapeDesc.vLocalOffset.y, ShapeDesc.vLocalOffset.z),
		PxQuat(ShapeDesc.vLocalRot.x, ShapeDesc.vLocalRot.y, ShapeDesc.vLocalRot.z, ShapeDesc.vLocalRot.w));

	pShape->setLocalPose(LocalPose);

	//Set Flag (Trigger or Simulation)
	//둘다 true불가능 
	if (ShapeDesc.bTrigger)
	{
		pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false); // 시물레이션 충돌체 설정값(물리연산)
		pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true); // 트리거 설정값
	}
	else
	{
		pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
		pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false); 

	}
	
#ifdef _DEBUG
	pShape->setFlag(PxShapeFlag::eVISUALIZATION, true);
#endif //_DEBUG
	
	Set_Filter(pShape, m_iFilterWord0, m_iFilterWord1);
	m_PhysicShapes.push_back(pShape);

	//pShape->release();
	return S_OK;
}

void CBasePhysic::Set_Filter(PxShape* pShape, _uint iWord0, _uint iWord1)
{
	if (pShape == nullptr) return;	
	PxFilterData FilterData;
	FilterData.word0 = iWord0;
	FilterData.word1 = iWord1;
	
	pShape->setSimulationFilterData(FilterData); //시뮬레이션 필터링 정보
	pShape->setQueryFilterData(FilterData);		// 쿼리 필터링 정보
	
}

void CBasePhysic::Free()
{
	__super::Free();

	//SetUp(Actor부착)전에 종료되는경우 삭제하기 위한 코드. 정상루트에서는 Actor에서 삭제됨 
	for(PxShape* pShape : m_PhysicShapes)
	{
		if(pShape)
			pShape->release();
	}
	m_PhysicShapes.clear();
}

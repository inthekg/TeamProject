#include "TestObject.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "AnimationPlayer.h"
#include "CCT.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CTestObject, CGameObject)

END_REFLECTION(CTestObject)
#endif // _DEBUG

CTestObject::CTestObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CGameObject(pDevice, pContext)
{
}

CTestObject::CTestObject(const CTestObject& Prototype)
	:CGameObject(Prototype)
{
}

HRESULT CTestObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTestObject::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	_uint iShaderID = 0;
	_uint iMaterialeID = 0;

	if (m_pShaderCom) iShaderID = m_pShaderCom->Get_ID();
	//TODO : Material 키값은 셰이더 구조 변경후 결정 
	if (m_pModelCom) iMaterialeID = 0;

	m_iSortKey = RenderHeader::MakeSortKey(iShaderID, iMaterialeID);

	_uint64 iIdleHashKey = Hash_String(L"Idle");
	auto iter =  m_AnimationPlayers.find(iIdleHashKey);	

	if(iter != m_AnimationPlayers.end())
		m_pCurrentAnimationPlayer = iter->second;

	return S_OK;

}

void CTestObject::Priority_Update(_float fTimeDelta)
{
}

void CTestObject::Update(_float fTimeDelta)
{
	if (m_pCurrentAnimationPlayer)
	{
		m_pCurrentAnimationPlayer->Play(fTimeDelta);
	}
}

void CTestObject::Physical_Update(_float fFixedTime)
{
	//_vector vMovedir = XMVectorSet(0.f, 0.f, 0.1f, 0.f);
	//if (m_pCCT)
	//{
	//	CCTResult result = m_pCCT->Move(vMovedir, fFixedTime);
	//	m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(&result.vSimulatedPosition));
	//}

}

void CTestObject::Late_Update(_float fTimeDelta)
{
	if (m_pCurrentAnimationPlayer)
	{
		m_pCurrentAnimationPlayer->Get_CurrentPose(m_CurrentPose);
		m_pModelCom->Update_Bones(&m_CurrentPose);
	}

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CTestObject::Render()
{
	_uint iNumMesh = m_pModelCom->Get_NumMeshes();

	//Static Mesh용 커맨드
	//for (_uint i = 0; i < iNumMesh; i++)
	//{
	//	CMesh* pMesh = m_pModelCom->Get_Mesh(i);
	//	CMaterial* pMaterialCom = m_pModelCom->Get_Material(i);
	//	if (pMesh == nullptr || pMaterialCom == nullptr) return E_FAIL;
	//	if (m_pShaderCom == nullptr) return E_FAIL;

	//	StaticCommand command{};
	//	command.header.etype = RENDER_TYPE::STATIC;
	//	command.header.iCommandSize = sizeof(StaticCommand);
	//	command.header.iSortKey = m_iSortKey;

	//	command.pVIBuffer = pMesh;
	//	command.pVS = m_pShaderCom->Get_VertexShader();
	//	command.pPS = m_pShaderCom->Get_PixelShader ();
	//	command.pLayout = m_pShaderCom->Get_InputLayout();
	//	command.pMaterial = pMaterialCom;
	//	command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

	//	m_pGameInstance->Submit_Static(command);
	//}


	//Skinned Mesh용 커맨드
	for (_uint i = 0; i < iNumMesh; i++)
	{
		CMesh* pMesh = m_pModelCom->Get_Mesh(i);
		CMaterial* pMaterialCom = m_pModelCom->Get_Material(i);
		if (pMesh == nullptr || pMaterialCom == nullptr) return E_FAIL;
		if (m_pShaderCom == nullptr) return E_FAIL;
	
		SkinnedCommand command{};
		command.header.etype = RENDER_TYPE::SKINNED;
		command.iBoneCount = pMesh->Get_BoneCount();
		command.header.iCommandSize = sizeof(SkinnedCommand);

		command.header.iSortKey = m_iSortKey;
	
		command.pVIBuffer = pMesh;
		command.pVS = m_pShaderCom->Get_VertexShader();
		command.pPS = m_pShaderCom->Get_PixelShader();
		command.pLayout = m_pShaderCom->Get_InputLayout();
		command.pMaterial = pMaterialCom;
		command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

		if (FAILED(m_pModelCom->Fill_SkinnedCommandBoneInfo(command, i)))
			continue;

		m_pGameInstance->Submit_Skinned(command);
	}
	return S_OK;
}

HRESULT CTestObject::Ready_Components()
{
	/* For.Com_Shader */
	if (FAILED(m_pGameInstance->Get_Shader(L"Shader_VtxAnimMesh", &m_pShaderCom))) 
		return E_FAIL;

	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Training"),
	//if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_FestivalLeader"),
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	/*auto& Animations = m_pModelCom->Get_Animations();
	m_iMaxAnimation = (_uint)Animations.size();
	_uint iNumBones = m_pModelCom->Get_NumBones();
	POSE* pBasePose = m_pModelCom->Get_BindPosePtr();
	m_CurrentPose = *pBasePose;

	for (_uint i = 0; i < m_iMaxAnimation; i++)
	{
		CAnimationPlayer* pAnimationPlayer = CAnimationPlayer::Create(Animations[i], iNumBones, pBasePose);

		if (pAnimationPlayer == nullptr) return E_FAIL;

		_uint64 hashKey = pAnimationPlayer->Get_Animation_StateHashNum();

		auto iter = m_AnimationPlayers.find(hashKey);
		if (iter != m_AnimationPlayers.end())
		{
			Safe_Release(iter->second);
		}

		m_AnimationPlayers[hashKey] = pAnimationPlayer;
	}*/

	//피직스 설정 예시 
	CCT::CONTROLLER_DESC CCTDesc;
	CCTDesc.eControllerType = CCT_TYPE::CAPSULE; // 캡슐 CCT를 만든다
	CCTDesc.fHeight = 4.75f;
	CCTDesc.fRadius = 1.2f;
	CCTDesc.fSlopeLimit = cosf(XMConvertToRadians(45.f)); // 올라갈수있는 경사각도
	CCTDesc.fStepOffset = 0.3f; //계단으로 인식할 높이. 이 높이보다 낮으면 올라감 
	CCTDesc.pOwner = this;
	XMStoreFloat3(&CCTDesc.vInitPos, m_pTransformCom->Get_State(STATE::POSITION));
	XMStoreFloat4(&CCTDesc.vInitQuat, m_pTransformCom->Get_Quat());

	CCTDesc.iFilterWord0 = ETOUI(FILTER_GROUP::PLAYER); //내 그룹
	CCTDesc.iFilterWord1 = ETOUI(FILTER_GROUP::MONSTER); // 충돌탐지할 상대그룸 
	
	//추가 콜라이더가 필요한 경우에 다음처럼 CCTDesc.Shapes에 추가하기. 
	//SHAPE_DESC shapeDesc;
	//shapeDesc.eType = GEOMETRY_TYPE::BOX;
	//shapeDesc.vHalfExtents = { 1.2f, 2.5f, 1.2f };
	//shapeDesc.vLocalOffset = { 0.f, 0.f, 0.f };
	//shapeDesc.bTrigger = true;
	//CCTDesc.Shapes.push_back(shapeDesc);

	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_CCT"),
		TEXT("Com_Physics"), reinterpret_cast<CComponent**>(&m_pCCT), &CCTDesc)))
		return E_FAIL;

	return S_OK;
}


CTestObject* CTestObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTestObject* pInstance = new CTestObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTestObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CTestObject::Clone(void* pArg)
{
	CTestObject* pInstance = new CTestObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTestObject::Free()
{
	__super::Free();

	for(auto& pair : m_AnimationPlayers)
	{
		Safe_Release(pair.second);
	}	
	m_AnimationPlayers.clear();

	Safe_Release(m_pCCT);
	Safe_Release(m_pShaderCom);
	Safe_Release(m_pModelCom);
}

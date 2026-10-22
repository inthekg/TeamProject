#include "Facial_Test.h"
#include "GameInstance.h"
#include "RenderStruct.h"
#include "AnimationPlayer.h"


CFacial_Test::CFacial_Test(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CGameObject(pDevice, pContext)
{
}

CFacial_Test::CFacial_Test(const CFacial_Test& Prototype)
	:CGameObject(Prototype)
{
}

HRESULT CFacial_Test::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CFacial_Test::Initialize(void* pArg)
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

	_uint64 iIdleHashKey = Hash_String(L"Hit_B");
	auto iter = m_AnimationPlayers.find(iIdleHashKey);

	if (iter != m_AnimationPlayers.end())
		m_pCurrentAnimationPlayer = iter->second;

	return S_OK;

}

void CFacial_Test::Priority_Update(_float fTimeDelta)
{
}

void CFacial_Test::Update(_float fTimeDelta)
{
	if (m_pCurrentAnimationPlayer)
	{
		m_pCurrentAnimationPlayer->Play(fTimeDelta);
	}
}

void CFacial_Test::Physical_Update(_float fFixedTime)
{
}

void CFacial_Test::Late_Update(_float fTimeDelta)
{
	if (m_pCurrentAnimationPlayer)
	{
		m_pCurrentAnimationPlayer->Get_CurrentPose(m_CurrentPose);
		m_pModelCom->Update_Bones(&m_CurrentPose);
	}

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CFacial_Test::Render()
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

HRESULT CFacial_Test::Ready_Components()
{
	/* For.Com_Shader */
	//Static용 Mesh
	//if (FAILED(m_pGameInstance->Get_Shader(L"Shader_VtxMesh", &m_pShaderCom))) return E_FAIL;
	
	if (FAILED(m_pGameInstance->Get_Shader(L"Shader_VtxAnimMesh", &m_pShaderCom)))
		return E_FAIL;

	//if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Buttler_Basic"),
	if (FAILED(__super::Add_Component(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Player_Facial"),
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
		return E_FAIL;

	//auto& Animations = m_pModelCom->Get_Animations();
	//m_iMaxAnimation = (_uint)Animations.size();
	//_uint iNumBones = m_pModelCom->Get_NumBones();
	//POSE* pBasePose = m_pModelCom->Get_BindPosePtr();
	//m_CurrentPose = *pBasePose;

	//for (_uint i = 0; i < m_iMaxAnimation; i++)
	//{
	//	CAnimationPlayer* pAnimationPlayer = CAnimationPlayer::Create(Animations[i], iNumBones, pBasePose);

	//	if (pAnimationPlayer == nullptr) return E_FAIL;

	//	_uint64 hashKey = pAnimationPlayer->Get_Animation_StateHashNum();

	//	auto iter = m_AnimationPlayers.find(hashKey);
	//	if (iter != m_AnimationPlayers.end())
	//	{
	//		Safe_Release(iter->second);
	//	}

	//	m_AnimationPlayers[hashKey] = pAnimationPlayer;
	//}

	return S_OK;
}


CFacial_Test* CFacial_Test::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CFacial_Test* pInstance = new CFacial_Test(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CFacial_Test");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CFacial_Test::Clone(void* pArg)
{
	CFacial_Test* pInstance = new CFacial_Test(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFacial_Test");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CFacial_Test::Free()
{
	__super::Free();

	for (auto& pair : m_AnimationPlayers)
	{
		Safe_Release(pair.second);
	}
	m_AnimationPlayers.clear();

	Safe_Release(m_pShaderCom);
	Safe_Release(m_pModelCom);
}

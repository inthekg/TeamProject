#include "DebugCapsule.h"
#include "VIBuffer_DebugCapsule.h"
#include "GameInstance.h"
#include "Shader.h"
#include "RenderStruct.h"
#include "Transform.h"

CDebugCapsule::CDebugCapsule(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CGameObject{ pDevice, pContext }
	, m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CDebugCapsule::Initialize_Prototype(const DEBUGCAPSULE_OBJ_DESC& OBJ_DESC)
{
	CGameObject::Initialize(nullptr);

	CVIBuffer_DebugCapsule::DEBUGCAPSULE_DESC VIBuffer_Desc;
	VIBuffer_Desc.fHalfHeight = OBJ_DESC.fHalfHeight;
	VIBuffer_Desc.fRadius = OBJ_DESC.fRadius;
	VIBuffer_Desc.iSegment = OBJ_DESC.iSegment;
	VIBuffer_Desc.vColor = OBJ_DESC.vColor;

	m_pVIBufferCom = CVIBuffer_DebugCapsule::Create(m_pDevice, m_pContext, VIBuffer_Desc);
	if (m_pVIBufferCom == nullptr) return E_FAIL;

	if (FAILED(m_pGameInstance->Get_Shader(L"Shader_LineWorld", &m_pShaderCom))) return E_FAIL;

	return S_OK;
}

HRESULT CDebugCapsule::Render()
{
	if (m_pShaderCom == nullptr) return E_FAIL;

	StaticCommand command{};
	command.header.etype = RENDER_TYPE::DEBUG;
	command.header.iCommandSize = sizeof(StaticCommand);
	command.header.iSortKey = RenderHeader::MakeSortKey(m_pShaderCom->Get_ID(), 0);

	command.pVIBuffer = m_pVIBufferCom;
	command.pVS = m_pShaderCom->Get_VertexShader();
	command.pPS = m_pShaderCom->Get_PixelShader();
	command.pLayout = m_pShaderCom->Get_InputLayout();
	command.pMaterial = nullptr;
	command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

	m_pGameInstance->Submit_Static(command);

	return S_OK;
}

void CDebugCapsule::Late_Update(_float fTimeDelta)
{
	m_pGameInstance->Add_RenderGroup(RENDERID::DEBUG, this);
}

void CDebugCapsule::Update_Pose(_fvector vPos, _fvector vQuat)
{
	m_pTransformCom->Set_State(STATE::POSITION, vPos);
	m_pTransformCom->Set_Quat(vQuat);
}

CDebugCapsule* CDebugCapsule::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const DEBUGCAPSULE_OBJ_DESC& OBJ_DESC)
{
	CDebugCapsule* pInstance = new CDebugCapsule(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype(OBJ_DESC)))
	{
		MSG_BOX("Failed to Created : CDebugCapsule");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CDebugCapsule::Free()
{
	__super::Free();
	Safe_Release(m_pVIBufferCom);
	Safe_Release(m_pShaderCom);
}

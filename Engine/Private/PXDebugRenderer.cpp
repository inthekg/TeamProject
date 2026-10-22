#include "PXDebugRenderer.h"
#include "VIBuffer_PXDebugLine.h"
#include "GameInstance.h"
#include "Shader.h"
#include "RenderStruct.h"

CPXDebugRenderer::CPXDebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject(pDevice, pContext), m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CPXDebugRenderer::Initialize_Prototype()
{
    if (FAILED(Ready_Components())) return E_FAIL;

    return S_OK;
}

void CPXDebugRenderer::Update()
{
    if (m_pVIBufferCom == nullptr) return;

    if (FAILED(m_pVIBufferCom->Update()))
    {
        MSG_BOX("Failed to Update : CVIBuffer_PXDebugLine");
    }

    m_pGameInstance->Add_RenderGroup(RENDERID::DEBUG, this);
}

HRESULT CPXDebugRenderer::Ready_Components()
{
	CVIBuffer* pTemp = nullptr;
    if (FAILED(m_pGameInstance->Get_VIBuffer(L"VIBUFFER_PXDEBUGLINE", &pTemp))) return E_FAIL;
	m_pVIBufferCom = static_cast<CVIBuffer_PXDebugLine*>(pTemp);    

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_Line", &m_pShaderCom))) return E_FAIL;
	return S_OK;
}


HRESULT CPXDebugRenderer::Render()
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
    //PXLine은 월드공간의 정점이 전달됨
    XMStoreFloat4x4(&command.worldMatrix , XMMatrixIdentity());
    m_pGameInstance->Submit_Static(command);

    return S_OK;
}


CPXDebugRenderer* CPXDebugRenderer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPXDebugRenderer* pInstance = new CPXDebugRenderer(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CPXDebugRenderer");
        Safe_Release(pInstance);
        return nullptr;
    }
    return pInstance;
}

void CPXDebugRenderer::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pShaderCom);
}   
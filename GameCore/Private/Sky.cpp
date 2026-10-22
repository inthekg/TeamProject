#include "Sky.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer.h"

CSky::CSky(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) : CGameObject{ pDevice, pContext }
{
}

CSky::CSky(const CSky& Prototype) : CGameObject{ Prototype }
{
}

HRESULT CSky::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CSky::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;
   
    return S_OK;
}

void CSky::Priority_Update(_float fTimeDelta)
{
}

void CSky::Update(_float fTimeDelta)
{
}

void CSky::Physical_Update(_float fFixedTime)
{
}

void CSky::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderGroup(RENDERID::CUBEMAP, this);
}

HRESULT CSky::Render()
{
    if (nullptr == m_pShaderCom || nullptr == m_pTextureCom || nullptr == m_pVIBufferCom)
        return E_FAIL;

    m_pShaderCom->Render();
    m_pTextureCom->Bind_ShaderResource(25);

    if (FAILED(m_pVIBufferCom->Bind_Resources()))
        return E_FAIL;

    if (FAILED(m_pVIBufferCom->Render()))
        return E_FAIL;

    ID3D11ShaderResourceView* pNullSRV = { nullptr };
    m_pContext->PSSetShaderResources(25, 1, &pNullSRV);

    return S_OK;
}

HRESULT CSky::Ready_Components()
{
    if (FAILED(m_pGameInstance->Get_Shader(TEXT("Shader_Sky"), &m_pShaderCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_Texture(TEXT("SkyBox"), &m_pTextureCom)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Get_VIBuffer(TEXT("VIBUFFER_CUBE"), &m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

CSky* CSky::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CSky* pInstance = new CSky(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CSky");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CSky::Clone(void* pArg)
{
    CSky* pInstance = new CSky(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CSky");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CSky::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
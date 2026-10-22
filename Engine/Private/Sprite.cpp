#include "Sprite.h"
#include "RenderStruct.h"
#include "GameInstance.h"
#include "Engine_Utility.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CSprite, CGameObject)
ADD_PROPERTY_SETTER(CSprite, m_strMaterialName, WString, Set_Material, _wstring)
END_REFLECTION(CSprite)
#endif

CSprite::CSprite(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CSprite::CSprite(const CSprite& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CSprite::Initialize_Prototype()
{
	Set_Name(L"Sprite");

	return S_OK;
}

HRESULT CSprite::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

    if (pArg)
    {
        SPRITE_DESC* pSpriteDesc = static_cast<SPRITE_DESC*>(pArg);
        if (!pSpriteDesc->strMaterialName.empty())
            Set_Material(pSpriteDesc->strMaterialName);
    }

	return S_OK;
}

void CSprite::Priority_Update(_float fTimeDelta)
{
}

void CSprite::Update(_float fTimeDelta)
{
}

void CSprite::Physical_Update(_float fFixedTime)
{
}

void CSprite::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderGroup(RENDERID::PRIORITY, this);
}

HRESULT CSprite::Render()
{
    if (m_pShaderCom == nullptr || m_pMaterialCom == nullptr || m_pVIBufferCom == nullptr)
    {
		MSG_BOX("Sprite : Shader, Material or VIBuffer is nullptr");
        return E_FAIL;
    }

    StaticCommand command{};
    command.header.etype = RENDER_TYPE::STATIC;
    command.header.iCommandSize = sizeof(StaticCommand);

    _uint iSortKey = RenderHeader::MakeSortKey(m_pShaderCom->Get_ID(), m_pMaterialCom->Get_ID());

    command.header.iSortKey = iSortKey;

    command.pVIBuffer = m_pVIBufferCom;
    command.pMaterial = m_pMaterialCom;

    command.pVS = m_pShaderCom->Get_VertexShader();
    command.pPS = m_pShaderCom->Get_PixelShader();
    command.pLayout = m_pShaderCom->Get_InputLayout();
    command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();
    m_pGameInstance->Submit_Static(command);
    return S_OK;
}

HRESULT CSprite::Set_Material(_wstring strMaterialName)
{
#ifdef _DEBUG
    if (m_strMaterialName == strMaterialName)
    {
        MSG_BOX("Same Name Not Change : Sprite");
        return S_FALSE;
    }
#endif // _DEBUG

    if (m_pMaterialCom) Safe_Release(m_pMaterialCom);

    if (FAILED(m_pGameInstance->Get_Material(strMaterialName, &m_pMaterialCom)))
    {
        MSG_BOX("Failed Get Material : Sprite");
        return E_FAIL;
    }

#ifdef _DEBUG
    m_strMaterialName = strMaterialName;
#endif

    return S_OK;
}

HRESULT CSprite::Ready_Components()
{	
    CVIBuffer* pBuffer = { nullptr };
	if (FAILED(m_pGameInstance->Get_VIBuffer(L"VIBUFFER_RECT",  &pBuffer))) return E_FAIL;
    m_pVIBufferCom = static_cast<CVIBuffer_Rect*>(pBuffer);

    if (FAILED(m_pGameInstance->Get_Shader(L"Shader_VtxTex", &m_pShaderCom))) return E_FAIL;
    
    //Material은 외부에서 바인딩 
    return S_OK;
}


CSprite* CSprite::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CSprite* pInstance = new CSprite(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CSprite");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CSprite::Clone(void* pArg)
{
    CSprite* pInstance = new CSprite(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CSprite");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CSprite::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pShaderCom);
    Safe_Release(m_pMaterialCom);

}
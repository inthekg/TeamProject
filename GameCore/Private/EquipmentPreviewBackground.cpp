#include "EquipmentPreviewBackground.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Material.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "RenderStruct.h"
#include "Engine_Utility.h"

CEquipmentPreviewBackground::CEquipmentPreviewBackground(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CEquipmentPreviewBackground::CEquipmentPreviewBackground(const CEquipmentPreviewBackground& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CEquipmentPreviewBackground::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEquipmentPreviewBackground::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	// 장비 UI가 열리기 전에는 렌더 그룹에 제출하지 않는다.
	m_bPreviewActive = false;

	return S_OK;
}

void CEquipmentPreviewBackground::Priority_Update(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);
}

void CEquipmentPreviewBackground::Update(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);
}

void CEquipmentPreviewBackground::Late_Update(_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (!m_bPreviewActive)
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::UI_PREVIEW_BACKGROUND, this);
}

HRESULT CEquipmentPreviewBackground::Render()
{
	if (!m_bPreviewActive)
		return S_OK;

	if (m_pShaderCom == nullptr || m_pVIBufferCom == nullptr || m_pSolidMaterial == nullptr || m_pPatternMaterial == nullptr)
		return E_FAIL;

	Submit_BackgroundCommand(m_pSolidMaterial, _float4(0.2f, 0.2f, 0.2f, 1.f), 0);
	Submit_BackgroundCommand(m_pPatternMaterial, _float4(1.f, 1.f, 1.f, 0.07058824f), 1);

	return S_OK;
}

HRESULT CEquipmentPreviewBackground::Ready_Components()
{
	CVIBuffer* pBuffer = { nullptr };

	if (FAILED(m_pGameInstance->Get_VIBuffer(L"VIBUFFER_RECT", &pBuffer)))
		return E_FAIL;

	m_pVIBufferCom = static_cast<CVIBuffer_Rect*>(pBuffer);

	if (FAILED(m_pGameInstance->Get_Shader(L"Shader_UI", &m_pShaderCom)))
		return E_FAIL;

	if (FAILED(Ready_Material(L"UI\\Common\\UIT_Img_Square.dds", &m_pSolidMaterial)))
		return E_FAIL;

	if (FAILED(Ready_Material(L"UI\\Common\\UIT_Texture.dds", &m_pPatternMaterial)))
		return E_FAIL;

	return S_OK;
}

HRESULT CEquipmentPreviewBackground::Ready_Material(const _wstring& strRelativeTexturePath, CMaterial** ppOutMaterial)
{
	if (ppOutMaterial == nullptr || strRelativeTexturePath.empty())
		return E_FAIL;

	*ppOutMaterial = nullptr;

	_wstring strTexturePath = m_pGameInstance->Get_ResourcePath() + L"\\" + strRelativeTexturePath;
	_wstring strTextureKey = Get_FileNameFromPath(strTexturePath);

	CTexture* pTexture = { nullptr };

	// UI를 열기 전에도 배경 객체가 생성되므로 필요한 텍스처를 직접 등록
	if (FAILED(m_pGameInstance->Get_Texture(strTextureKey, &pTexture)))
	{
		if (FAILED(m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strTexturePath)))
			return E_FAIL;

		if (FAILED(m_pGameInstance->Get_Texture(strTextureKey, &pTexture)))
			return E_FAIL;
	}

	CMaterial* pMaterial = CMaterial::Create(m_pDevice, m_pContext);

	if (pMaterial == nullptr)
	{
		Safe_Release(pTexture);
		return E_FAIL;
	}

	pMaterial->Input_Texture(TEXTURE_TYPE::BASE_COLOR, pTexture);

	Safe_Release(pTexture);

	*ppOutMaterial = pMaterial;

	return S_OK;
}

void CEquipmentPreviewBackground::Submit_BackgroundCommand(CMaterial* pMaterial, const _float4& vColor, _uint iSortOrder)
{
	if (pMaterial == nullptr)
		return;

	UICommand Command = {};

	Command.header.etype = RENDER_TYPE::UI;
	Command.header.iCommandSize = sizeof(UICommand);
	Command.header.iSortKey = iSortOrder;

	// 프리뷰 카메라 기준으로 배치된 월드 공간 Rect를 사용한다.
	Command.worldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();

	Command.pVIBuffer = m_pVIBufferCom;
	Command.pVS = m_pShaderCom->Get_VertexShader();
	Command.pPS = m_pShaderCom->Get_PixelShader();
	Command.pLayout = m_pShaderCom->Get_InputLayout();
	Command.pMaterial = pMaterial;

	Command.vColor = vColor;
	Command.vColorLT = vColor;
	Command.vColorRT = vColor;
	Command.vColorRB = vColor;
	Command.vColorLB = vColor;

	Command.vUVRect = _float4(0.f, 0.f, 1.f, 1.f);
	Command.fFillAmount = 1.f;
	Command.iUseTexture = 1;
	Command.iBlendMode = 0;

	m_pGameInstance->Submit_UI(Command);
}

CEquipmentPreviewBackground* CEquipmentPreviewBackground::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEquipmentPreviewBackground* pInstance = new CEquipmentPreviewBackground(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CEquipmentPreviewBackground");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CEquipmentPreviewBackground::Clone(void* pArg)
{
	CEquipmentPreviewBackground* pInstance = new CEquipmentPreviewBackground(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEquipmentPreviewBackground");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEquipmentPreviewBackground::Free()
{
	__super::Free();

	Safe_Release(m_pPatternMaterial);
	Safe_Release(m_pSolidMaterial);
	Safe_Release(m_pVIBufferCom);
	Safe_Release(m_pShaderCom);
}
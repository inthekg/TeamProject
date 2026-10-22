#include "Decal.h"
#include "GameInstance.h"
#include "VIBuffer_Cube.h"
#include "Effect_Emitter.h"
#include "Texture.h"
#include "Engine_Utility.h"

namespace
{
	struct DECAL_CB
	{
		_float4x4	DecalWorld;
		_float4x4	DecalInvWorld;
		_float4		vTint;
		_float2		vTiling;			_float2		vUVOffset;
		_float		fAngleFadeStart;	_float		fAngleFadeEnd;		_float		fAlbedoWeight;		_float		fNormalWeight;
		_float		fORMSWeight;		_float		fEmissiveWeight;	_float		fEmissiveIntensity;	_int		iHasAlbedo;
		_int		iHasNormal;			_int		iHasORMS;			_int		iHasEmissive;		_int		iHasMask;			
	};

	static constexpr _uint DECAL_CB_SLOT = 13;
}

CDecal::CDecal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject { pDevice, pContext }
{
	XMStoreFloat4x4(&m_matWorld, XMMatrixIdentity());
}

HRESULT CDecal::Initialize(void* pArg)
{
	m_pBox = CVIBuffer_Cube::Create(m_pDevice, m_pContext);
	if (m_pBox == nullptr)
		return E_FAIL;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.ByteWidth = sizeof(DECAL_CB);
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;

	if (FAILED(m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pDecalCB)))
		return E_FAIL;

	return S_OK;
}

void CDecal::Activate(void* pArg)
{
	m_fGlobalAlpha = 1.f;
}

void CDecal::InActivate()
{

}

void CDecal::Update(_float fTimeDelta)
{

}

void CDecal::Set_Desc(const DecalDesc& tDesc)
{
	m_tDesc = tDesc;

	Update_Texture(tDesc.strAlbedoPath, m_strAlbedoPath, &m_pAlbedoTex);
	Update_Texture(tDesc.strNormalPath, m_strNormalPath, &m_pNormalTex);
	Update_Texture(tDesc.strORMSPath, m_strORMSPath, &m_pORMSTex);
	Update_Texture(tDesc.strEmissivePath, m_strEmissivePath, &m_pEmissiveTex);
	Update_Texture(tDesc.strMaskPath, m_strMaskPath, &m_pMaskTex);
}

void CDecal::Update_Texture(const string& strPath, string& strCache, CTexture** ppOutTex)
{
	if (strPath == strCache)
		return;

	strCache = strPath;

	Safe_Release(*ppOutTex);
	*ppOutTex = nullptr;

	if (strPath.empty())
		return;

	int iLen = MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, nullptr, 0);
	wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
	if (iLen > 0)
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, &strWPath[0], iLen);

	*ppOutTex = Load_Cached_Texture(strWPath);
}

CTexture* CDecal::Load_Cached_Texture(const wstring& strWPath)
{
	if (strWPath.empty() || m_pGameInstance == nullptr)
		return nullptr;

	wstring strPathCopy = strWPath;
	wstring strName = Get_FileNameFromPath(strPathCopy);

	CTexture* pTexture = nullptr;

	if (FAILED(m_pGameInstance->Get_Texture(strName, &pTexture)))
	{
		m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strWPath);
		m_pGameInstance->Get_Texture(strName, &pTexture);
	}

	return pTexture;
}

HRESULT CDecal::Render()
{
	if (m_pBox == nullptr || m_pBundle == nullptr || m_pDecalCB == nullptr)
		return S_OK;

	if (m_pBundle->pDecalVS == nullptr || m_pBundle->pDecalPS == nullptr)
		return S_OK;
	if (m_fGlobalAlpha <= 0.f)
		return S_OK;

	DECAL_CB cb{};
	_matrix matW = XMLoadFloat4x4(&m_matWorld);
	_matrix matInvW = XMMatrixInverse(nullptr, matW);

	XMStoreFloat4x4(&cb.DecalWorld, XMMatrixTranspose(matW));
	XMStoreFloat4x4(&cb.DecalInvWorld, XMMatrixTranspose(matInvW));

	cb.vTint = _float4(m_tDesc.fTint[0], m_tDesc.fTint[1], m_tDesc.fTint[2], m_tDesc.fTint[3] * m_fGlobalAlpha);
	cb.vTiling = _float2(m_tDesc.fTiling[0], m_tDesc.fTiling[1]);
	cb.vUVOffset = _float2(m_tDesc.fUVOffset[0], m_tDesc.fUVOffset[1]);
	cb.fAngleFadeStart = m_tDesc.fAngleFadeStart;
	cb.fAngleFadeEnd = m_tDesc.fAngleFadeEnd;
	cb.fAlbedoWeight = m_tDesc.fAlbedoWeight;
	cb.fNormalWeight = m_tDesc.fNormalWeight;
	cb.fORMSWeight = m_tDesc.fORMSWeight;
	cb.fEmissiveWeight = m_tDesc.fEmissiveWeight;
	cb.fEmissiveIntensity = m_tDesc.fEmissiveIntensity;
	cb.iHasAlbedo = (m_pAlbedoTex != nullptr) ? 1 : 0;
	cb.iHasNormal = (m_pNormalTex != nullptr) ? 1 : 0;
	cb.iHasORMS = (m_pORMSTex != nullptr) ? 1 : 0;
	cb.iHasEmissive = (m_pEmissiveTex != nullptr) ? 1 : 0;
	cb.iHasMask = (m_pMaskTex != nullptr) ? 1 : 0;

	m_pContext->UpdateSubresource(m_pDecalCB, 0, nullptr, &cb, 0, 0);

	m_pContext->VSSetShader(m_pBundle->pDecalVS, nullptr, 0);
	m_pContext->PSSetShader(m_pBundle->pDecalPS, nullptr, 0);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->HSSetShader(nullptr, nullptr, 0);
	m_pContext->DSSetShader(nullptr, nullptr, 0);
	m_pContext->IASetInputLayout(m_pBundle->pDecalLayout);

	m_pContext->VSSetConstantBuffers(DECAL_CB_SLOT, 1, &m_pDecalCB);
	m_pContext->PSSetConstantBuffers(DECAL_CB_SLOT, 1, &m_pDecalCB);

	ID3D11ShaderResourceView* pSRVs[5] =
	{
		m_pAlbedoTex ? m_pAlbedoTex->Get_SRV() : nullptr,
		m_pNormalTex ? m_pNormalTex->Get_SRV() : nullptr,
		m_pORMSTex ? m_pORMSTex->Get_SRV() : nullptr,
		m_pEmissiveTex ? m_pEmissiveTex->Get_SRV() : nullptr,
		m_pMaskTex ? m_pMaskTex->Get_SRV() : nullptr
	};
	m_pContext->PSSetShaderResources(0, 5, pSRVs);
	
	m_pBox->Bind_Resources();
	m_pBox->Render();

	return S_OK;
}

CDecal* CDecal::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CDecal* pInstance = new CDecal(pDevice, pContext);
	if (FAILED(pInstance->Initialize(nullptr)))
	{
		MSG_BOX("Failed to Create : CDecal");
		Safe_Release(pInstance);
	}
	
	return pInstance;
}

void CDecal::Free()
{
	Safe_Release(m_pAlbedoTex);
	Safe_Release(m_pNormalTex);
	Safe_Release(m_pORMSTex);
	Safe_Release(m_pEmissiveTex);
	Safe_Release(m_pMaskTex);

	Safe_Release(m_pDecalCB);
	Safe_Release(m_pBox);

	__super::Free();
}
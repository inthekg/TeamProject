#include "Texture.h"
#include "Engine_Utility.h"


CTexture::CTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CResource{ pDevice, pContext }
{
}

HRESULT CTexture::Initialize_Prototype(_wstring* pTextureName, _wstring* pTextureFilePath)
{
	if (pTextureName == nullptr || pTextureFilePath == nullptr) return E_FAIL;
	Set_Name(*pTextureName);
	
	ID3D11ShaderResourceView* pSRV = { nullptr };

	_tchar		szEXT[MAX_PATH] = { };
	const _tchar* szFilePath = pTextureFilePath->c_str();
		
	_wsplitpath_s(szFilePath, nullptr, 0, nullptr, 0, nullptr, 0, szEXT, MAX_PATH);

	HRESULT		hr = {};

	if (false == lstrcmp(szEXT, TEXT(".dds")))
	{
		hr = CreateDDSTextureFromFile(m_pDevice, szFilePath, nullptr, &pSRV);
	}
	else if (false == lstrcmp(szEXT, TEXT(".tga")))
	{
		hr = E_FAIL;
	}
	else
	{
		hr = CreateWICTextureFromFile(m_pDevice, szFilePath, nullptr, &pSRV);
	}

	if (FAILED(hr))
	{
		OutputDebugStringW((L"[Texture MISSING] " + *pTextureName + L"\n").c_str());
		Safe_Release(m_pTexture);
		return E_FAIL;
	}

	m_pTexture = pSRV;
	return S_OK;
}



void CTexture::Bind_ShaderResource(_uint iSlot)
{
	m_pContext->PSSetShaderResources(iSlot, 1, &m_pTexture);
}

CTexture* CTexture::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring* pTextureName, _wstring* pTextureFilePath)
{
	CTexture* pInstance = new CTexture(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pTextureName, pTextureFilePath)))
	{
		//TODO : 수정하기. 일단 텍스쳐없을때 에러 발생막아놓음 나중에 주석풀기 
		//MSG_BOX("Failed to Created : CTexture");
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CTexture::Free()
{
	__super::Free();
	Safe_Release(m_pTexture);
}

#include "ComputeShader.h"

CComputeShader::CComputeShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CResource { pDevice, pContext }
{

}

HRESULT CComputeShader::Initialize_Prototype(COMPUTE_SHADER_DESC& Desc)
{
	if (Desc.strShaderName.empty() || Desc.strCSFilePath.empty())
	{
		MSG_BOX("ComputeShader Create Fail : Name/Path Empty");
		return E_FAIL;
	}
	Set_Name(Desc.strShaderName);

	_uint iHlslFlag = {};
#ifdef _DEBUG
	iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	iHlslFlag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif

	ID3D10Blob* pCSBlob = { nullptr };
	ID3D10Blob* pErrorBlob = { nullptr };

	if (FAILED(D3DCompileFromFile(Desc.strCSFilePath.c_str(), nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, Desc.strCSFunName.c_str(),
		"cs_5_0", iHlslFlag, 0, &pCSBlob, &pErrorBlob)))
	{
		if (pErrorBlob)
			MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(), "Compute Shader Compile Error", MB_OK);
		else
			MessageBoxA(nullptr, "Unknown error or file not found.", "Compute Shader Compile Error", MB_OK);
		Safe_Release(pErrorBlob);
		Safe_Release(pCSBlob);

		return E_FAIL;
	}

	HRESULT hr = m_pDevice->CreateComputeShader(pCSBlob->GetBufferPointer(),
		pCSBlob->GetBufferSize(), nullptr, &m_pComputeShader);

	Safe_Release(pErrorBlob);
	Safe_Release(pCSBlob);

	if (FAILED(hr))
	{
		MSG_BOX("Failed to Create Compute Shader");
		return E_FAIL;
	}
	return S_OK;
}

void CComputeShader::Bind_ConstantBuffer(_uint iSlot, ID3D11Buffer* pCB)
{
	m_pContext->CSSetConstantBuffers(iSlot, 1, &pCB);
}

void CComputeShader::Bind_SRV(_uint iSlot, ID3D11ShaderResourceView* pSRV)
{
	m_pContext->CSSetShaderResources(iSlot, 1, &pSRV);
}

void CComputeShader::Bind_Sampler(_uint iSlot, ID3D11SamplerState* pSampler)
{
	m_pContext->CSSetSamplers(iSlot, 1, &pSampler);
}

void CComputeShader::Bind_UAV(_uint iSlot, ID3D11UnorderedAccessView* pUAV, _uint iInitailCount)
{
	m_pContext->CSSetUnorderedAccessViews(iSlot, 1, &pUAV, &iInitailCount);
}

void CComputeShader::Dispatch(_uint iGroupX, _uint iGroupY, _uint iGroupZ)
{
	m_pContext->CSSetShader(m_pComputeShader, nullptr, 0);
	m_pContext->Dispatch(iGroupX, iGroupY, iGroupZ);
}

void CComputeShader::DispatchIndirect(ID3D11Buffer* pArgsBuffer, _uint iAlignedOffset)
{
	m_pContext->CSSetShader(m_pComputeShader, nullptr, 0);
	m_pContext->DispatchIndirect(pArgsBuffer, iAlignedOffset);
}

void CComputeShader::Unbind_UAV(_uint iSlot, _uint iCount)
{
	ID3D11UnorderedAccessView* nullUAV[8] = {};
	_uint initCounts[8] = {};
	if (iCount > 8) iCount = 8;
	m_pContext->CSSetUnorderedAccessViews(iSlot, iCount, nullUAV, initCounts);
}

void CComputeShader::Unbind_SRV(_uint iSlot, _uint iCount)
{
	ID3D11ShaderResourceView* nullSRV[8] = {};
	if (iCount > 8) iCount = 8;
	m_pContext->CSSetShaderResources(iSlot, iCount, nullSRV);
}

CComputeShader* CComputeShader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, 
	COMPUTE_SHADER_DESC& Desc)
{
	CComputeShader* pInstance = new CComputeShader(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype(Desc)))
	{
		MSG_BOX("Failed to Created : CComputeShader");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CComputeShader::Free()
{
	__super::Free();
	Safe_Release(m_pComputeShader);
}


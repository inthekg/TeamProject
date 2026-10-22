#include "VIBuffer_GPUParticle.h"

CVIBuffer_GPUParticle::CVIBuffer_GPUParticle(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{

}

HRESULT CVIBuffer_GPUParticle::Initialize_Prototype()
{
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	return S_OK;
}

void CVIBuffer_GPUParticle::Set_DrawSource(ID3D11ShaderResourceView* pPoolSRV,
	ID3D11ShaderResourceView* pAliveSRV, ID3D11Buffer* pIndirectArgs,
	_uint iFixedCount, ID3D11Buffer* pCameraCB)
{
	m_pPoolSRV = pPoolSRV;
	m_pAliveSRV = pAliveSRV;
	m_pIndirectArgs = pIndirectArgs;
	m_iFixedCount = iFixedCount;
	m_pCameraCB = pCameraCB;
}

HRESULT CVIBuffer_GPUParticle::Bind_Resources()
{
	ID3D11Buffer* nullVB = { nullptr };
	_uint iStride = 0, iOffset = 0;

	m_pContext->IASetVertexBuffers(0, 1, &nullVB, &iStride, &iOffset);
	m_pContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	m_pContext->VSSetShaderResources(0, 1, &m_pPoolSRV);
	if (m_pAliveSRV)
		m_pContext->VSSetShaderResources(1, 1, &m_pAliveSRV);
	m_pContext->VSSetShaderResources(3, 1, &m_pScale3DSRV);

	if (m_pCameraCB)
	{
		m_pContext->VSSetConstantBuffers(7, 1, &m_pCameraCB);
		m_pContext->PSSetConstantBuffers(7, 1, &m_pCameraCB);
	}

	if (m_pRenderCB)
	{
		m_pContext->VSSetConstantBuffers(6, 1, &m_pRenderCB);
		m_pContext->PSSetConstantBuffers(6, 1, &m_pRenderCB);
	}

	m_pContext->PSSetShaderResources(2, 1, &m_pDiffuseSRV);
	m_pContext->PSSetShaderResources(4, 1, &m_pNoiseASRV);
	m_pContext->PSSetShaderResources(5, 1, &m_pNoiseBSRV);
	m_pContext->PSSetShaderResources(6, 1, &m_pShapeMaskSRV);
	m_pContext->PSSetShaderResources(7, 1, &m_pEmissiveBSRV);
	m_pContext->PSSetShaderResources(8, 1, &m_pDissolveSRV);
	m_pContext->PSSetShaderResources(SRV_RTV_DEPTH_SLOT, 1, &m_pDepthSRV);
	m_pContext->PSSetShaderResources(10, 1, &m_pSceneSRV);
	m_pContext->PSSetShaderResources(11, 1, &m_pDistortNoiseSRV);
	m_pContext->PSSetShaderResources(12, 1, &m_pEmissiveASRV);

	return S_OK;
}

HRESULT CVIBuffer_GPUParticle::Render()
{
	if (m_pIndirectArgs)
		m_pContext->DrawInstancedIndirect(m_pIndirectArgs, 0);
	else
		m_pContext->DrawInstanced(4, m_iFixedCount, 0, 0);

	ID3D11ShaderResourceView* nullSRV[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	m_pContext->VSSetShaderResources(0, 2, nullSRV);
	m_pContext->VSSetShaderResources(3, 1, nullSRV);
	m_pContext->PSSetShaderResources(2, 1, nullSRV);
	m_pContext->PSSetShaderResources(4, 5, nullSRV);
	m_pContext->PSSetShaderResources(SRV_RTV_DEPTH_SLOT, 1, nullSRV);
	m_pContext->PSSetShaderResources(10, 1, nullSRV);
	m_pContext->PSSetShaderResources(11, 1, nullSRV);
	m_pContext->PSSetShaderResources(12, 1, nullSRV);
	return S_OK;
}

CVIBuffer_GPUParticle* CVIBuffer_GPUParticle::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVIBuffer_GPUParticle* pInstance = new CVIBuffer_GPUParticle(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVIBuffer_GPUParticle");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_GPUParticle::Free()
{
	__super::Free();
}
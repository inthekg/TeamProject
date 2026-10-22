#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_GPUMesh final : public CVIBuffer
{
private:
	CVIBuffer_GPUMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CVIBuffer_GPUMesh() = default;

public:
	virtual HRESULT Initialize_Prototype();

	HRESULT			Build_Quad();
	HRESULT			Load_StaticMesh(const _wstring& strFilePath, _uint iMeshIndex);
	_uint			Get_IndexCount() const { return m_iNumIndices; }

	void			Set_DrawSource(ID3D11ShaderResourceView* pPoolSRV, ID3D11ShaderResourceView* pAliveSRV,
						ID3D11Buffer* pIndirectArgs, _uint iFixedCount, ID3D11Buffer* pCameraCB);
	void			Set_Diffuse(ID3D11ShaderResourceView* pDiffuseSRV) { m_pDiffuseSRV = pDiffuseSRV; }
	void            Set_NoiseA(ID3D11ShaderResourceView* pSRV) { m_pNoiseASRV = pSRV; }
	void            Set_NoiseB(ID3D11ShaderResourceView* pSRV) { m_pNoiseBSRV = pSRV; }
	void			Set_ShapeMask(ID3D11ShaderResourceView* pSRV) { m_pShapeMaskSRV = pSRV; }
	void			Set_EmissiveA(ID3D11ShaderResourceView* pSRV) { m_pEmissiveASRV = pSRV; }
	void            Set_EmissiveB(ID3D11ShaderResourceView* pSRV) { m_pEmissiveBSRV = pSRV; }
	void            Set_Dissolve(ID3D11ShaderResourceView* pSRV) { m_pDissolveSRV = pSRV; }
	void            Set_Crack(ID3D11ShaderResourceView* pSRV) { m_pCrackSRV = pSRV; }
	void			Set_Scale3D(ID3D11ShaderResourceView* pSRV) { m_pScale3DSRV = pSRV; }
	void			Set_Render(ID3D11Buffer* pRenderCB) { m_pRenderCB = pRenderCB; }
	void			Set_Scene(ID3D11ShaderResourceView* pSceneSRV) { m_pSceneSRV = pSceneSRV; }
	void			Set_DistortNoise(ID3D11ShaderResourceView* pDistortSRV) { m_pDistortNoiseSRV = pDistortSRV; }
	virtual HRESULT	Bind_Resources() override;
	virtual HRESULT Render() override;

private:
	ID3D11ShaderResourceView*	m_pPoolSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pAliveSRV = { nullptr };
	ID3D11Buffer*				m_pIndirectArgs = { nullptr };
	ID3D11Buffer*				m_pCameraCB = { nullptr };
	ID3D11Buffer*				m_pRenderCB = { nullptr };
	ID3D11ShaderResourceView*	m_pDiffuseSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pNoiseASRV = { nullptr };
	ID3D11ShaderResourceView*	m_pNoiseBSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pShapeMaskSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pEmissiveASRV = { nullptr };
	ID3D11ShaderResourceView*	m_pEmissiveBSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pDissolveSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pCrackSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pScale3DSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pSceneSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pDistortNoiseSRV = { nullptr };
	_uint						m_iFixedCount = {};

public:
	static CVIBuffer_GPUMesh*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*			Clone(void* pArg) override { return nullptr; }
	virtual void				Free() override;
};

NS_END
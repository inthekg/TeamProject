#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)

struct TRAIL_DRAW_DESC
{
	_float4			vColor = { 1.f, 1.f, 1.f, 1.f };
	_float			fWidth = { 0.1f };
	_float			fTailWidthScale = { -1.f };
	_float3			vCamPos = { 0.f, 0.f, 0.f };
	_bool			bFadeOut = { true };

	_bool			bSpline = { true };
	_float			fSinSpeed = { 1.f };
	_float			fWaveAmplitude = { 0.f };
	_int			iTexMode = { 0 };
	_float			fTime = { 0.f };

	const _float4*	pColorLUT = { nullptr };
	_uint			iColorLUTSize = { 0 };
	_bool			bColorCurve = { false };

	_bool			bDualSocket = { false };
	const vector<_float3>* pHalf = { nullptr };

	_float2			vBaseTile = { 1.f, 1.f };
	_float2			vBaseSpeed = { 0.f, 0.f };
	_float2			vNoiseTile = { 0.2f, 1.f };
	_float2			vNoiseSpeed = { -0.1f, 0.f };
	_float			fBasePower = { 1.f };
	_float			fBaseDensity = { 1.f };
	_float			fNoisePower = { 1.f };
	_float			fNoiseDensity = { 1.f };
	_float			fEmiIntensity = { 1.f };
	_bool			bUseAlphaY = { false };
	_float			fAlphaYPower = { 15.f };
	_float			fAlphaYPower2 = { 15.f };
	_float			fAlphaYPivot = { 0.5f };
	_int			iAlphaYMode = { true };

	_bool			bUseDistortion = { false };
	_float			fDistortion = { 10.f };
	_float2			vScreen = { 1.f, 1.f };

	const _float4x4* pView = { nullptr };
	const _float4x4* pProj = { nullptr };
};

class ENGINE_DLL CVIBuffer_Trail : public CVIBuffer
{
public:
	static const _uint MAX_INPUT_POINTS = 256;
	static const _uint MAX_OUTPUT_POINTS = 512;
	static const _uint MAX_SUBDIV = 16;

private:
	CVIBuffer_Trail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Trail(const CVIBuffer_Trail& Prototype);
	virtual ~CVIBuffer_Trail() = default;

public:
	virtual HRESULT		Initialize_Prototype() override;
	virtual HRESULT		Initialize(void* pArg) override;

	virtual HRESULT		Bind_Resources() override;
	virtual HRESULT		Render() override;

public:
	void				Update_Ribbon(const vector<_float3>& Points, const TRAIL_DRAW_DESC& Desc);
	void				Set_DiffuseSRV(ID3D11ShaderResourceView* pSRV) { m_pDiffuseSRV = pSRV; }
	void				Set_NoiseSRV(ID3D11ShaderResourceView* pSRV) { m_pNoiseSRV = pSRV; }
	void				Set_SceneSRV(ID3D11ShaderResourceView* pSRV) { m_pSceneSRV = pSRV; }
private:
	void				Build_SplinePoints(const vector<_float3>& In, vector<_float3>& Out, _bool bSpline);

private:
	ID3D11Buffer*				m_pCameraCB = { nullptr };
	_uint						m_iDrawVertexCount = { 0 };
	ID3D11ShaderResourceView*	m_pDiffuseSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pNoiseSRV = { nullptr };
	ID3D11ShaderResourceView*	m_pSceneSRV = { nullptr };

public:
	static CVIBuffer_Trail* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*		Clone(void* pArg) override;
	virtual	void			Free() override; 
};

NS_END
#pragma once
#include "PoolableObject.h"
#include "Effect_Define.h"

NS_BEGIN(Engine)

class	CVIBuffer_Trail;
struct	EMITTER_SHADER_BUNDLE;
class	CMaterial;

class ENGINE_DLL CEffect_Trail final: public CPoolableObject
{
public:
	static const _uint LUT_SIZE = 64;

private:
	CEffect_Trail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CEffect_Trail() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void	Activate(void* pArg = nullptr) override;
	virtual void	InActivate() override;
	virtual void	Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

public:
	void			Bind_SharedShaders(const EMITTER_SHADER_BUNDLE* pBundle) { m_pBundle = pBundle; }
	_int			Get_BlendMode() const { return m_iBlendMode; }
	void			Set_Params(const TrailModuleDesc& tTrail);
	void			Set_Color(const _float4& vColor) { m_vColor = vColor; }
	void			Set_ColorCurve(const _float4* pColors, _bool bEnabled);
	void			Set_Texture(const string& strPath);
	void			Set_NoiseTexture(const string& strPath);
	void			Set_SceneSRV(ID3D11ShaderResourceView* pSRV);
	void			Set_Anchor(const _float3& vAnchor) { m_vAnchor = vAnchor; m_bHasAnchor = true; m_bDualSocket = false; }
	void			Set_Anchor2(const _float3& vTop, const _float3& vRot);
	void			Set_Anchor2_Arc(const vector<_float3>& Centers, const vector<_float3>& Halves);
	void			Begin_Fade() { m_bFading = true; }
	void			Reset();

private:
	void			Trim_ToMaxLength();

private:
	CVIBuffer_Trail*				m_pBuffer = { nullptr };
	const EMITTER_SHADER_BUNDLE*	m_pBundle = { nullptr };

	vector<_float3> m_Points;
	vector<_float3> m_Half;
	vector<_float>	m_PointAge;

	vector<_float3> m_FrameCenters;
	vector<_float3> m_FrameHalves;

	_float3			m_vAnchor = { 0.f, 0.f, 0.f };
	_float3			m_vAnchorHalf = { 0.f, 0.f, 0.f };
	_bool			m_bHasAnchor = { false };
	_bool			m_bFading = { false };
	_float			m_fTime = { 0.f };

	_float			m_fWidth = { 0.1f };
	_float			m_fMaxLength = { 2.f };
	_float			m_fTailWidthScale = { 1.f };
	_float			m_fTrailLife = { 0.3f };
	_float			m_fPointInterval = { 0.05f };
	_bool			m_bFadeOut = { true };
	_bool			m_bSpline = { true };
	_float			m_fSinSpeed = { 1.f };
	_float			m_fWaveAmplitude = { 0.f };
	_int			m_iTexMode = { 0 };
	_bool			m_bDualSocket = { false };
	_float4			m_vColor = { 1.f, 1.f, 1.f, 1.f };

	_float2			m_vBaseTile = { 1.f, 1.f };
	_float2			m_vBaseSpeed = { 0.f, 0.f };
	_float2			m_vNoiseTile = { 0.2f, 1.f };
	_float2			m_vNoiseSpeed = { -0.1f, 0.f };
	_float			m_fBasePower = { 1.f };
	_float			m_fBaseDensity = { 1.f };
	_float			m_fNoisePower = { 1.f };
	_float			m_fNoiseDensity = { 1.f };
	_float			m_fEmiIntensity = { 1.f };
	_bool			m_bUseAlphaY = { false };
	_float			m_fAlphaYPower = { 15.f };
	_float			m_fAlphaYPower2 = { 15.f };
	_float			m_fAlphaYPivot = { 0.5f };
	_int			m_iAlphaYMode = { 1 };
	_int			m_iBlendMode = { 1 };
	_bool			m_bUseDistortion = { false };
	_float			m_fDistortion = { 10.f };

	_float4			m_ColorLUT[LUT_SIZE] = {};
	_bool			m_bColorCurve = { false };

	CMaterial*		m_pMaterial = { nullptr };
	string			m_strTexturePath;
	CMaterial*		m_pNoiseMaterial = { nullptr };
	string			m_strNoiseTexPath;

public:
	static CEffect_Trail*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override { return nullptr; }
	virtual void			Free() override;
};

NS_END
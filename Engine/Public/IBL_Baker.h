#pragma once
#include "Base.h"

NS_BEGIN(Engine)
class CComputeShader;
class CTexture_UAV;

class ENGINE_DLL CIBL_Baker final :	public CBase
{
public :
	enum class IBL { IRR_SIZE = 32, PREF_SIZE = 128, PREF_MIPS = 5, LUT_SIZE = 256 };

private :
	CIBL_Baker(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CIBL_Baker() = default;

public:
	ID3D11ShaderResourceView*	Get_Irradiance_SRV() const;
	ID3D11ShaderResourceView*	Get_Prefiltered_SRV() const;
	ID3D11ShaderResourceView*	Get_BRDFLUT_SRV() const;
	float						Get_MaxMip() const { return static_cast<float>(ETOUI(IBL::PREF_MIPS) - 1); }
	_bool						Is_Baked() const { return m_bBaked; }

public :
	HRESULT						Initialize();
	HRESULT						Bake(ID3D11ShaderResourceView* pSkyCubeSRV);

private :
	HRESULT						Ready_ComputeShaders();
	HRESULT						Ready_OutputTextures();
	HRESULT						Ready_PrefilterCB();
	HRESULT						Ready_LinearSampler();

private :
	ID3D11Device*				m_pDevice = { nullptr };
	ID3D11DeviceContext*		m_pContext = { nullptr };
	
	CComputeShader*				m_pCSIrradiance = { nullptr };
	CComputeShader*				m_pCSPrefilter = { nullptr };
	CComputeShader*				m_pCSBRDFLut = { nullptr };

	CTexture_UAV*				m_pIrradiance = { nullptr };
	CTexture_UAV*				m_pPrefiltered = { nullptr };
	CTexture_UAV*				m_pBRDFLut = { nullptr };

	ID3D11Buffer*				m_pPrefilterCB = { nullptr };
	ID3D11SamplerState*			m_pLinearSampler = { nullptr };

	_bool                       m_bBaked = { false };

public :
	static CIBL_Baker* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END
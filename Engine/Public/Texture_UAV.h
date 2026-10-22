#pragma once
#include "Base.h"

// 컴퓨트 쉐이더 내 직접 쓸 수 있는 텍스쳐 ( 2D / 큐브맵 / 밉체인 )
NS_BEGIN(Engine)

class ENGINE_DLL CTexture_UAV final : public CBase
{
public :
	typedef struct tagTextureUAVDesc
	{
		_uint		iWidth	= {};
		_uint		iHeight = {};
		DXGI_FORMAT eFormat = { DXGI_FORMAT_R16G16B16A16_FLOAT };
		_uint		iArraySize = { 1 }; // 6 -> 큐브맵 
		_uint		iMipLevels = { 1 };
		_bool		bCubeMap = { false };
	}TEX_UAV_DESC;

private :
	CTexture_UAV(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CTexture_UAV() = default;

public:
	ID3D11Texture2D*			Get_Texture2D() const { return m_pTexture2D; }
	ID3D11ShaderResourceView*	Get_SRV() const { return m_pSRV; }
	ID3D11UnorderedAccessView*	Get_UAV(_uint iMip = 0) const { return (iMip < m_UAVs.size()) ? m_UAVs[iMip] : nullptr; }
	const	TEX_UAV_DESC&		Get_Desc() const { return m_tDesc; }
	_uint						Get_MipLevels() const { return m_tDesc.iMipLevels; }

public :
	HRESULT						Initialize(const TEX_UAV_DESC& Desc);

private :
	ID3D11Device*						m_pDevice = { nullptr };
	ID3D11DeviceContext*				m_pContext = { nullptr };	
	ID3D11Texture2D*					m_pTexture2D = { nullptr };
	ID3D11ShaderResourceView*			m_pSRV = { nullptr };
	vector<ID3D11UnorderedAccessView*>	m_UAVs;				// 밉 당 1개
	TEX_UAV_DESC						m_tDesc = { };

public :
	static CTexture_UAV* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const TEX_UAV_DESC& Desc);
	virtual void		 Free() override;
};

NS_END
#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CStructuredBuffer : public CBase
{
public:
	typedef struct tagStructuredBufferDesc
	{
		_uint	iElementStride = {}; 
		_uint	iElementCount = {};
		void*	pInitialData = { nullptr };
		_bool	bAppend = { false };
	} SB_DESC;

protected:
	CStructuredBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CStructuredBuffer() = default;

public:
	HRESULT						Initialize(const SB_DESC& Desc);

public:
	ID3D11Buffer*				Get_Buffer() const { return m_pBuffer; }
	ID3D11UnorderedAccessView*	Get_UAV() const { return m_pUAV; }
	ID3D11ShaderResourceView*	Get_SRV() const { return m_pSRV; }

	// Animation_CS
	void						Update_Buffer(const void* pData, _uint iElementCount);
	void						Update_Buffer_Region(const void* pData, _uint iStartElement, _uint iElementCount);

	HRESULT						Readback(void* pDst, _uint iByteSize);
	void						Copy_Count(ID3D11Buffer* pDst, _uint iDstByteOffset);
protected:
	ID3D11Device*				m_pDevice = { nullptr };
	ID3D11DeviceContext*		m_pContext = { nullptr };
	
	ID3D11Buffer*				m_pBuffer = { nullptr };
	ID3D11UnorderedAccessView*	m_pUAV = { nullptr };
	ID3D11ShaderResourceView*	m_pSRV = { nullptr };
	ID3D11Buffer*				m_pStaging = { nullptr };

	_uint						m_iByteWidth = {};
	_uint						m_iElementStride = {};
	_uint						m_iElementCount = {};

public:
	static CStructuredBuffer*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const SB_DESC& Desc);
	virtual void				Free() override;
};

NS_END
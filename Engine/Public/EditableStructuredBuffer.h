#pragma once
#include "StructuredBuffer.h"

NS_BEGIN(Engine)

//업데이트 가능한 구조체 버퍼. 단, 빈번한 갱신이 필요하다면 ConstantBuffer 사용하기. 
class ENGINE_DLL CEditableStructuredBuffer final : public CStructuredBuffer
{
protected:
	CEditableStructuredBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CEditableStructuredBuffer() = default;

public:
	HRESULT						Initialize(const SB_DESC& Desc);

	//빈번한 갱신 금지 
	void						Update_Buffer(void* pData, UINT dataCount);
	void						BindVS(_uint iSlot);
	void						BindPS(_uint iSlot);

	_uint						m_iElementStride	= {};
	_uint						m_iMaxElementCount	= {};

public:
	static CEditableStructuredBuffer*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const SB_DESC& Desc);
	virtual void						Free() override;

};

NS_END
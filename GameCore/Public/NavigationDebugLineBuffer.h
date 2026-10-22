#pragma once

#include "Core_Defines.h"
#include "VIBuffer.h"

#ifdef _DEBUG

NS_BEGIN(GameCore)

class CNavigationDebugLineBuffer final : public Engine::CVIBuffer
{
private:
	CNavigationDebugLineBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CNavigationDebugLineBuffer() = default;

public:
	HRESULT Initialize_Prototype(_uint iMaxVertexCount);
	HRESULT Set_Lines(const vector<Engine::VTXCOLOR>& Lines);
	_uint Get_LineVertexCount() const { return m_iLineVertexCount; }

public:
	virtual HRESULT Bind_Resources() override;
	virtual HRESULT Render() override;
	virtual Engine::CComponent* Clone(void* pArg) override { return nullptr; }

private:
	_uint m_iLineVertexCount = { 0 };

public:
	static CNavigationDebugLineBuffer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iMaxVertexCount);
	virtual void Free() override;
};

NS_END

#endif
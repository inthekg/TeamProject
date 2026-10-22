#pragma once
#include "Camera.h"

NS_BEGIN(Engine)
class ENGINE_DLL CCamera_Main final : public CCamera
{
protected:
	CCamera_Main(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera_Main(const CCamera_Main& Prototype) = delete;
	virtual ~CCamera_Main() = default;

public:
	virtual HRESULT			Initialize(void* pArg);

public:
	static CCamera_Main*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual CGameObject*	Clone(void* pArg) { return nullptr; }
	virtual void			Free() override;
};

NS_END
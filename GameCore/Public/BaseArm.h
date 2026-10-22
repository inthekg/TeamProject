#pragma once
#include "LegionArm.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CBaseArm final : public CLegionArm
{
protected:
	CBaseArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CBaseArm() = default;

public:
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Update(_float fTimeDelta) override;

public:
	static CBaseArm* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual void			Free() override;
};

NS_END
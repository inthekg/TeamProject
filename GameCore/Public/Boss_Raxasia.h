#pragma once
#include "AIObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CBoss_Raxasia final : public CAIObject
{
private:
	CBoss_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoss_Raxasia(const CBoss_Raxasia& Prototype);
	virtual ~CBoss_Raxasia() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	static CBoss_Raxasia* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END

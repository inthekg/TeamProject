#pragma once
#include "Projectile.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CMissile final : public CProjectile
{
private:
	CMissile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMissile(const CMissile& Prototype);
	virtual ~CMissile() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual void			Activate(void* pArg = nullptr) override;
	virtual void			InActivate() override;

protected:
	virtual const _wstring& Get_ModelProtoTag() const override;
	virtual void			On_ImpactFx(_fvector vPos) override;
	virtual void			On_Explode(_fvector vPos) override;
	virtual void			Collect_Trails(vector<TRAIL_SPEC>& Out) const override;
	virtual MODE			Get_Mode() const override;

private:
	string					m_strTrailKey = { "Effect_Missile_Trail" };
	_float					m_fExplodeRadius = { 3.f };

public:
	static CMissile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
};

NS_END

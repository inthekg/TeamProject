#pragma once
#include "Projectile.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CBullet final : public CProjectile
{
private:
	CBullet(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBullet(const CBullet& Prototype);
	virtual ~CBullet() = default;

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
	virtual void			Collect_Trails(vector<TRAIL_SPEC>& Out) const override;
	virtual MODE			Get_Mode() const override { return MODE::BULLET; }

private:
	string					m_strTrailKey = { "Effect_Bullet_Trail" };

public:
	static CBullet* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
};

NS_END

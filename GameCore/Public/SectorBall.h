#pragma once
#include "Projectile.h"

NS_BEGIN(Engine)
class CEffect;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CSectorBall final : public CProjectile
{
private:
	CSectorBall(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSectorBall(const CSectorBall& Prototype);
	virtual ~CSectorBall() = default;

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

	virtual MODE			Get_Mode() const override { return MODE::FAN; }
	virtual const char* Get_HitCue() const override { return "SE_NPC_Raxasia_SK_PJ_Spark_Wave_Explo_Cue"; }
private:
	string					m_strTrailKey = {};
	CEffect*				m_pEffectFx = {};
	CEffect*				m_pTrailFx = {};

public:
	static CSectorBall* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
};

NS_END

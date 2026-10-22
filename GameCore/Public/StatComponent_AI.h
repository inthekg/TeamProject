#pragma once
#include "Base.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)

class CStatComponent_AI : public CBase
{
private :
	CStatComponent_AI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxGroggy, _float fGroggyDecayDelay, _float fGroggyDecayRate, _float fGroggyCooldownMax);
	virtual ~CStatComponent_AI() = default;

public :
	_float		Get_MaxHP() const { return m_fMaxHP; }
	_float		Get_CurrentHP() const { return m_fCurrentHP; }
	_float		Get_HPPercent() const { return m_fMaxHP > 0.f ? m_fCurrentHP / m_fMaxHP : 0.f; }
	_bool		Is_Dead() const { return m_fCurrentHP <= 0.f; }

	_float		Get_MaxGroggy() const { return m_fMaxGroggy; }
	_float		Get_CurrentGroggy() const { return m_fCurrentGroggy; }
	_float		Get_GroggyPercent() const { return m_fMaxGroggy > 0.f ? m_fCurrentGroggy / m_fMaxGroggy : 0.f; }
	_bool		Is_GroggyFull() const { return m_fCurrentGroggy >= m_fMaxGroggy; }

	_bool		Is_Groggy() const { return m_bGroggy; }
	_bool		Is_OnGroggyCooldown() const { return m_fGroggyCooldown > 0.f; }
	_float		Get_GroggyCooldown() const { return m_fGroggyCooldown; }

	_bool		Is_Staggerable() const { return m_bStaggerable; }
	_bool		Is_FatalDying() const { return m_bFatalDying; }

	_float		Get_SinceHit() const { return m_fSinceHit; }
	HIT_TYPE	Get_LastHitType() const { return m_eLastHitType; }

public:
	void		Apply_Damage(_float fDamage) { m_fCurrentHP = max(m_fCurrentHP - fDamage, 0.f); }
	void		Set_CurrentHP(_float fHP) { m_fCurrentHP = min(max(fHP, 0.f), m_fMaxHP); }
	void		Revive() { m_fCurrentHP = m_fMaxHP; }

	void		Add_Groggy(_float fAmount) { m_fCurrentGroggy = min(m_fCurrentGroggy + fAmount, m_fMaxGroggy); }
	void		Reduce_Groggy(_float fAmount) { m_fCurrentGroggy = max(m_fCurrentGroggy - fAmount, 0.f); }
	void		Reset_Groggy() { m_fCurrentGroggy = 0.f; }

	void		Set_Staggerable(_bool bStaggerable) { m_bStaggerable = bStaggerable; }
	void		Set_FatalDying(_bool bFatalDying) { m_bFatalDying = bFatalDying; }
	void		Set_SinceHit(_float fSinceHit) { m_fSinceHit = fSinceHit; }
	void		Set_LastHitType(HIT_TYPE eType) { m_eLastHitType = eType; }

public :	
	void	Update_StatComponent(_float fTimeDelta);

	void	On_Hit(HIT_TYPE eType);
	_bool	Try_EnterGroggy();
	void	Leave_Groggy();
	void	Reset_OnSpawn();

private:
	_float		m_fMaxHP = {};
	_float		m_fCurrentHP = {};

	_float		m_fMaxGroggy = {};
	_float		m_fCurrentGroggy = {};

	_float		m_fGroggyDecayDelay = {};
	_float		m_fGroggyDecayRate = {};
	_float		m_fSinceHit = {};

	_float		m_fGroggyCooldown = {};
	_float		m_fGroggyCooldownMax = { 1.f };

	_bool		m_bGroggy = {};
	_bool		m_bStaggerable = {};
	_bool		m_bFatalDying = {};

	HIT_TYPE	m_eLastHitType = { HIT_TYPE::End };

public :
	static CStatComponent_AI* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxGroggy, _float fGroggyDecayDelay, _float fGroggyDecayRate, _float fGroggyCooldownMax);
	virtual void Free() override;
};

NS_END
#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)
class CStatComponent : public CComponent
{
private:
	CStatComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxStamina, _float fMaxFableStatmina);
	virtual ~CStatComponent() = default;

public:
	_float					Get_MaxHP() const { return m_fMaxHP; }
	_float					Get_CurrentHP() const { return m_fCurrentHP; }

	_float					Get_MaxStamina() const { return m_fMaxStamina; }
	_float					Get_CurrentStamina() const { return m_fCurrentStamina; }

	_float					Get_HPPercent() const { return m_fCurrentHP / m_fMaxHP; }
	_float 					Get_StaminaPercent() const { return m_fCurrentStamina / m_fMaxStamina; }

	_float					Get_FablePercent() const { return m_fCurFable_Stamina / FABLE_STAMINA_MAX; }
	_bool					Has_Fable() const { return m_fCurFable_Stamina >= FABLE_AMOUNT; }
	_bool					Has_FableEX() const { return m_fCurFable_Stamina >= FABLE_EX_AMOUNT; }

	_float					Get_MaxFableStamina() const { return FABLE_STAMINA_MAX; }
	_float					Get_CurrentFableStamina() const { return m_fCurFable_Stamina; }

	//Has_Stamina 사용하지않음! 예전 코드때문에 놔둠. 스테미나 체크는 IsEmpty로만 하기 
	_bool					Has_Stamina(_float fAmount) const { return m_fCurrentStamina >= fAmount; }
	_bool 					Is_StaminaEmpty() const { return m_fCurrentStamina <= 0.f; }
	_bool					Is_Dead() const { return m_fCurrentHP <= 0.f; }

public:
	void					Apply_Damage(_float fDamage) { m_fCurrentHP = max(m_fCurrentHP - fDamage, 0.f); }
	void					Consume_Stamina(_float fAmount);

	void					Drain_Stamina(_float fAmountPerSec, _float fTimeDelta) { Consume_Stamina(fAmountPerSec * fTimeDelta); }
	void					Regen_Stamina(_float fAmount) { m_fCurrentStamina = min(m_fCurrentStamina + fAmount, m_fMaxStamina); }

	//void					Consume_Fable(_float fAmount) { m_fCurFable_Stamina = max(m_fCurFable_Stamina - fAmount, 0.f); }
	
	void					Consume_Fable();
	void					Consume_FableEX();
	void					Regen_Fable(_float fAmount) { m_fCurFable_Stamina = min(m_fCurFable_Stamina + fAmount, FABLE_STAMINA_MAX); }

	void					Set_CurrentHP(_float fHP) { m_fCurrentHP = min(max(fHP, 0.f), m_fMaxHP); }
	void					Revive() { m_fCurrentHP = m_fMaxHP; }

public:
	void					Update(_float fTimeDelta);
	virtual void			Clear() override;

public:
	/* Heal 관련 */
	_uint					Get_PotionCount() const { return m_iPotionCount; }
	_float					Get_PotionChargeAmount() const { return m_fPotionCharge_Current; }
	_bool					Has_Potion() const { return m_iPotionCount > 0; }

	void					Charge_Potion(_float fDamage);
	_bool					Consume_Potion();
	void					Fill_Potion_Max();

	_bool					Is_Healing() const { return m_fHealRemain > 0.f; }
	void					Heal();
	void					Cancel_Heal() { m_fHealRemain = 0.f; m_fHealRate = 0.f; }

private:
	static constexpr _float FABLE_REGEN_RATE		= { 0.5f }; // 기본 0.1f
	static constexpr _float FABLE_AMOUNT			= { 1.f };
	static constexpr _float FABLE_EX_AMOUNT			= { 3.f };
	static constexpr _float FABLE_STAMINA_MAX		= { 3.f };

	static constexpr _float STAMINA_REGEN_RATE		= { 25.f };
	static constexpr _float STAMINA_REGEN_DELAY		= { 1.f };
	static constexpr _float STAMINA_REGEN_EMTPY_DELAY = { 2.f };
	 
	/* Heal 관련 */
	static constexpr unsigned int POTION_MAX_COUNT	= { 5 };
	//데미지 비례 포션 차는 비율 
	static constexpr _float	PORTION_CHARGE_RATE		= { 0.01f };
	//포션 차는 초당 속도
	static constexpr _float PORTION_CHARGE_SPEED	= { 0.8f };

	//힐이 될 HP비율 
	static constexpr _float HEAL_RATE				= { 0.4f };
	static constexpr _float HEAL_DURATION			= { 1.2f };
private:
	_float					m_fMaxHP				= {};
	_float					m_fCurrentHP			= {};
							
	_float					m_fMaxStamina			= {};
	_float					m_fCurrentStamina		= {};
	

	_float					m_fCurFable_Stamina		= {};

	_bool					m_bEmptyWait			= {};
	_float					m_fRegenDelay			= { 0.f };
	_float					m_fStaminaDelayTimer	= { 1.f };

private:
	_uint					m_iPotionCount			= { POTION_MAX_COUNT };
	_float					m_fPotionCharge_Target	= { 0.f };
	_float					m_fPotionCharge_Current = { 0.f };

	_float					m_fHealTotalAmount		= { 0.f };
	_float 					m_fHealRemain			= { 0.f };
	_float 					m_fHealRate				= { 0.f };

public:
	static CStatComponent* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxStamina, _float fMaxFableStamina = 3.f);
	virtual void Free() override;
};

NS_END
#include "StatComponent.h"

CStatComponent::CStatComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxStamina, _float fMaxFableStamina)
	: CComponent{pDevice, pContext}
	, m_fMaxHP(fMaxHP)
	, m_fCurrentHP(fMaxHP)
	, m_fMaxStamina(fMaxStamina)
	, m_fCurrentStamina(fMaxStamina)
	, m_fCurFable_Stamina(fMaxFableStamina)
{
	m_fHealTotalAmount = fMaxHP * HEAL_RATE;
}

void CStatComponent::Consume_Stamina(_float fAmount)
{
	m_fCurrentStamina -= fAmount;

	if (m_fCurrentStamina <= 0.f)
	{
		m_bEmptyWait = true;
		m_fCurrentStamina = 0.f;
	}
	m_fStaminaDelayTimer = 0.f;
}

void CStatComponent::Consume_Fable()
{
	m_fCurFable_Stamina = max(0.f, m_fCurFable_Stamina- FABLE_AMOUNT);
}

void CStatComponent::Consume_FableEX()
{
	m_fCurFable_Stamina = max(0.f, m_fCurFable_Stamina- FABLE_EX_AMOUNT);
}

void CStatComponent::Update(_float fTimeDelta)
{
	/* Potion */
	if (m_fPotionCharge_Current < m_fPotionCharge_Target)
	{
		m_fPotionCharge_Current += PORTION_CHARGE_SPEED * fTimeDelta;

		if(m_fPotionCharge_Current > m_fPotionCharge_Target )
			m_fPotionCharge_Current = m_fPotionCharge_Target;

		if (m_fPotionCharge_Current >= 1.f)
		{
			m_iPotionCount = min(m_iPotionCount + 1, POTION_MAX_COUNT);
			m_fPotionCharge_Current = 0.f;
			m_fPotionCharge_Target = 0.f;
		}
	}


	if (m_fHealRemain > 0.f)
	{
		_float fHealAmount = min(m_fHealRemain, m_fHealRate * fTimeDelta);
		m_fCurrentHP = min(m_fCurrentHP + fHealAmount, m_fMaxHP);
		m_fHealRemain -= fHealAmount;
	}

	if(m_fCurFable_Stamina < FABLE_STAMINA_MAX)
	{
		Regen_Fable(FABLE_REGEN_RATE * fTimeDelta);
	}

	if (m_fCurrentStamina >= m_fMaxStamina) return;

	if (m_bEmptyWait)
	{
		if (m_fStaminaDelayTimer < STAMINA_REGEN_EMTPY_DELAY)
		{
			m_fStaminaDelayTimer += fTimeDelta;
			if (m_fStaminaDelayTimer >= STAMINA_REGEN_EMTPY_DELAY)
			{
				m_bEmptyWait = false;
			}
		}
		return;
	}

	if (m_fStaminaDelayTimer < STAMINA_REGEN_DELAY) m_fStaminaDelayTimer += fTimeDelta;
	else
	{
		Regen_Stamina(STAMINA_REGEN_RATE * fTimeDelta);
	}
}

void CStatComponent::Clear()
{
	m_fCurrentHP = m_fMaxHP;
	m_fCurrentStamina = m_fMaxStamina;
	m_fCurFable_Stamina = FABLE_STAMINA_MAX;
	
	m_iPotionCount = POTION_MAX_COUNT;

	m_fHealRemain = 0.f;
	m_fHealRate = 0.f;
	m_fPotionCharge_Current = 0.f;
	m_fPotionCharge_Target = 0.f;
}

/* Heal 관련 */
void CStatComponent::Charge_Potion(_float fDamage)
{
	if (m_iPotionCount > 0 || fDamage <= 0.f) return;
	m_fPotionCharge_Target = min(1.f, m_fPotionCharge_Target + fDamage * PORTION_CHARGE_RATE);
}

_bool CStatComponent::Consume_Potion()
{
	if (m_iPotionCount <= 0) return false;

	--m_iPotionCount;
	return true;
}

void CStatComponent::Fill_Potion_Max()
{ 
	m_iPotionCount = POTION_MAX_COUNT;
	m_fPotionCharge_Current = 0.f;	
	m_fPotionCharge_Target = 0.f;
}

void CStatComponent::Heal()
{
	//이미 회복중이면 가산 
	m_fHealRemain += m_fHealTotalAmount;
	m_fHealRate = m_fHealTotalAmount / HEAL_DURATION;
}

CStatComponent* CStatComponent::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxStamina, _float fMaxFableStamina)
{
	CStatComponent* pInstance = new CStatComponent(pDevice, pContext, fMaxHP, fMaxStamina, fMaxFableStamina);
	if (pInstance == nullptr)
	{
		MessageBox(0, L"CStatComponent Created Failed", L"System Error", MB_OK);
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CStatComponent::Free()
{
	__super::Free();
}

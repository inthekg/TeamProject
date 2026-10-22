#include "StatComponent_AI.h"

CStatComponent_AI::CStatComponent_AI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxGroggy, _float fGroggyDecayDelay, _float fGroggyDecayRate, _float fGroggyCooldownMax)
	: m_fMaxHP(fMaxHP)
	, m_fCurrentHP(fMaxHP)
	, m_fMaxGroggy(fMaxGroggy)
	, m_fCurrentGroggy(0.f)
	, m_fGroggyDecayDelay(fGroggyDecayDelay)
	, m_fGroggyDecayRate(fGroggyDecayRate)
	, m_fGroggyCooldownMax(fGroggyCooldownMax)
{
}

void CStatComponent_AI::Update_StatComponent(_float fTimeDelta)
{
	if (m_fGroggyCooldown > 0.f)
		m_fGroggyCooldown = max(m_fGroggyCooldown - fTimeDelta, 0.f);

	m_fSinceHit += fTimeDelta;

	if (m_fSinceHit >= m_fGroggyDecayDelay && m_fCurrentGroggy > 0.f)
		Reduce_Groggy(m_fGroggyDecayRate * fTimeDelta);
}


void CStatComponent_AI::On_Hit(HIT_TYPE eType)
{
	m_fSinceHit = 0.f;
	m_eLastHitType = eType;
}


_bool CStatComponent_AI::Try_EnterGroggy()
{
	if (m_fGroggyCooldown > 0.f)
		return false;

	m_bGroggy = true;
	return true;
}

void CStatComponent_AI::Leave_Groggy()
{
	m_bGroggy = false;
	m_fGroggyCooldown = m_fGroggyCooldownMax;
}


void CStatComponent_AI::Reset_OnSpawn()
{
	m_fCurrentHP = m_fMaxHP;
	m_fCurrentGroggy = 0.f;
	m_fSinceHit = 0.f;
	m_fGroggyCooldown = 0.f;
	m_bGroggy = false;
	m_bStaggerable = false;
	m_bFatalDying = false;
	m_eLastHitType = HIT_TYPE::End;
}

CStatComponent_AI* CStatComponent_AI::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _float fMaxHP, _float fMaxGroggy, _float fGroggyDecayDelay, _float fGroggyDecayRate, _float fGroggyCooldownMax)
{
	CStatComponent_AI* pInstance = new CStatComponent_AI(pDevice, pContext,
		fMaxHP, fMaxGroggy, fGroggyDecayDelay, fGroggyDecayRate, fGroggyCooldownMax);
	if (nullptr == pInstance)
	{
		MessageBox(0, L"CStatComponent_AI Created Failed", L"System Error", MB_OK);
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CStatComponent_AI::Free()
{
	__super::Free();
}

#include "BuffComponent.h"
#include "GameInstance.h"
#include "Effect.h"

CBuffComponent::CBuffComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

HRESULT CBuffComponent::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	BUFFCOM_DESC* pDesc = static_cast<BUFFCOM_DESC*>(pArg);
	m_pOwnerWorld = pDesc->pOwnerWorld;

	return S_OK;
}


void CBuffComponent::Apply(BUFF_TYPE eType, const BUFF_DESC& tDesc)
{
	if (eType >= BUFF_TYPE::End || tDesc.fDuration <= 0.f)
		return;

	BUFF_SLOT& tSlot = m_Slots[ETOUI(eType)];

	_float fRemain = (true == tSlot.bActive && true == tDesc.bStackTime)
		? tSlot.fRemain + tDesc.fDuration : max(tSlot.fRemain, tDesc.fDuration);

	if (tDesc.fMaxDuration > 0.f)
		fRemain = min(fRemain, tDesc.fMaxDuration);

	tSlot.fRemain = fRemain;
	tSlot.fDuration = max(tSlot.fDuration, fRemain);
	tSlot.fValue = tDesc.fValue;
	tSlot.pFollowBone = tDesc.pFollowBone;
	tSlot.vEffectOffset = tDesc.vEffectOffset;

	// 새로 켜질때만 이펙트 재생 
	if (false == tSlot.bActive)
	{
		tSlot.bActive = true;
		Start_Effect(eType, tDesc.szEffectTag);
	}

	Refresh_Multiplier();
}


void CBuffComponent::Remove(BUFF_TYPE eType)
{
	if (eType >= BUFF_TYPE::End)
		return;

	BUFF_SLOT& tSlot = m_Slots[ETOUI(eType)];
	if (false == tSlot.bActive)
		return;

	Stop_Effect(eType);
	tSlot = {};

	Refresh_Multiplier();
}


void CBuffComponent::Update(_float fTimeDelta)
{
	_bool bExpired = false;

	for (_uint i = 0; i < ETOUI(BUFF_TYPE::End); ++i)
	{
		BUFF_SLOT& tSlot = m_Slots[i];
		if (false == tSlot.bActive)
			continue;

		tSlot.fRemain -= fTimeDelta;
		if (tSlot.fRemain > 0.f)
			continue;

		Stop_Effect(static_cast<BUFF_TYPE>(i));
		tSlot = {};
		bExpired = true;
	}

	if (true == bExpired)
		Refresh_Multiplier();
}

void CBuffComponent::Clear()
{
	for (_uint i = 0; i < ETOUI(BUFF_TYPE::End); ++i)
	{
		Stop_Effect(static_cast<BUFF_TYPE>(i));
		m_Slots[i] = {};
	}

	Refresh_Multiplier();
}

_bool CBuffComponent::Is_Active(BUFF_TYPE eType) const
{
	if (eType >= BUFF_TYPE::End)
		return false;

	return m_Slots[ETOUI(eType)].bActive;
}

_float CBuffComponent::Get_Remain(BUFF_TYPE eType) const
{
	if (eType >= BUFF_TYPE::End)
		return 0.f;

	return m_Slots[ETOUI(eType)].fRemain;
}

_float CBuffComponent::Get_Value(BUFF_TYPE eType) const
{
	if (eType >= BUFF_TYPE::End)
		return 0.f;

	const BUFF_SLOT& tSlot = m_Slots[ETOUI(eType)];

	return (true == tSlot.bActive) ? tSlot.fValue : 0.f;
}

_float CBuffComponent::Get_Ratio(BUFF_TYPE eType) const
{
	if (eType >= BUFF_TYPE::End)
		return 0.f;

	const BUFF_SLOT& tSlot = m_Slots[ETOUI(eType)];
	if (false == tSlot.bActive || tSlot.fDuration <= 0.f)
		return 0.f;

	return tSlot.fRemain / tSlot.fDuration;
}

void CBuffComponent::Refresh_Multiplier()
{
	const BUFF_SLOT& tAttack = m_Slots[ETOUI(BUFF_TYPE::AttackUp)];
	const BUFF_SLOT& tTaken = m_Slots[ETOUI(BUFF_TYPE::DamageTakenUp)];

	m_fAttackMul = 1.f + ((true == tAttack.bActive) ? tAttack.fValue : 0.f);
	m_fDamageTakenMul = 1.f + ((true == tTaken.bActive) ? tTaken.fValue : 0.f);
}

void CBuffComponent::Start_Effect(BUFF_TYPE eType, const _char* szTag)
{
	if (nullptr == szTag || !m_Slots[ETOUI(eType)].bActive)
		return;

	m_Slots[ETOUI(eType)].pEffect = m_pGameInstance->Play_Effect(szTag, m_Slots[ETOUI(eType)].pFollowBone, m_pOwnerWorld, m_Slots[ETOUI(eType)].vEffectOffset);
}

void CBuffComponent::Stop_Effect(BUFF_TYPE eType)
{
	BUFF_SLOT& tSlot = m_Slots[ETOUI(eType)];
	if (nullptr == tSlot.pEffect)
		return;

	tSlot.pEffect->Stop();
	tSlot.pEffect = nullptr;
}

CBuffComponent* CBuffComponent::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg)
{
	CBuffComponent* pInstance = new CBuffComponent(pDevice, pContext);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CBuffComponent");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBuffComponent::Free()
{
	Clear();
	__super::Free();
}

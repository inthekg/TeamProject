#include "UIViewModelBase.h"

HRESULT CUIViewModelBase::Initialize(void* pArg)
{
	return S_OK;
}

_bool CUIViewModelBase::Is_Dirty() const
{
	return m_bDirty;
}

void CUIViewModelBase::Clear_Dirty()
{
	m_bDirty = false;
}

void CUIViewModelBase::Update_Damage(_float fCurrentHP, _float fTimeDelta)
{    
	// 최초 프레임은 비교할 이전 HP만 저장한다.
	if (false == m_bDamageTrackingInitialized)
	{
		m_fPreviousHP = fCurrentHP;
		m_bDamageTrackingInitialized = true;
		return;
	}

    const _float fDamage = m_fPreviousHP - fCurrentHP;
    m_fPreviousHP = fCurrentHP;

    // 새로운 피해가 들어오면 기존 피해량에 누적하고 표시 시간을 3초로 초기화한다.
    if (fDamage > FLT_EPSILON)
    {
        m_fAccumulatedDamage += fDamage;
        m_fDamageRemainTime = DAMAGE_VISIBLE_TIME;
        m_bDamageVisible = true;
        Make_Dirty();
        return;
    }

    if (false == m_bDamageVisible)
        return;

    m_fDamageRemainTime = max(m_fDamageRemainTime - fTimeDelta, 0.f);

    if (m_fDamageRemainTime > 0.f)
        return;

    // 3초 동안 추가 피해가 없으면 텍스트를 숨기고 누적값을 초기화한다.
    m_bDamageVisible = false;
    m_fAccumulatedDamage = 0.f;
    Make_Dirty();
}

void CUIViewModelBase::Set_FatalAvailable(_bool bAvailable)
{
    if (m_bFatalAvailable == bAvailable)
        return;

    m_bFatalAvailable = bAvailable;
    Make_Dirty();
}

void CUIViewModelBase::Make_Dirty()
{
	m_bDirty = true;
}

_float CUIViewModelBase::Clamp01(_float fValue)
{
	return min(1.f, max(0.f, fValue));
}

_bool CUIViewModelBase::Is_SamePosition(const _float2& a, const _float2& b)
{
	if (fabs(a.x - b.x) <= 0.001f && fabs(a.y - b.y) <= 0.001f)
		return true;

	return false;
}

void CUIViewModelBase::Free()
{
	__super::Free();
}
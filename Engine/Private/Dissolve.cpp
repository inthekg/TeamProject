#include "Dissolve.h"

CDissolve::CDissolve()
	: CComponent(nullptr, nullptr)
{

}

HRESULT CDissolve::Initialize(void* pArg)
{
	if (pArg != nullptr)
		m_tDesc = *reinterpret_cast<DISSOLVE_DESC*>(pArg);
	return S_OK;
}

void CDissolve::Begin(DISSOLVE_MODE eMode)
{
	m_eMode = eMode;
	m_bPlaying = true;
	m_bFinished = false;
	m_fTimer = 0.f;
	m_fProgress = 0.f;
}

void CDissolve::Update(_float fTimeDelta)
{
	if (m_bPlaying == false || m_bFinished)
		return;

	m_fTimer += fTimeDelta;

	if (m_fTimer >= m_tDesc.fStartDelay)
	{
		_float fDuration = (m_tDesc.fDuration > 1e-4f) ? m_tDesc.fDuration : 1e-4f;
		_float fNorm = (m_fTimer - m_tDesc.fStartDelay) / fDuration;

		if (fNorm > 1.f)
			fNorm = 1.f;

		m_fProgress = fNorm;
	}

	if (m_fTimer >= m_tDesc.fStartDelay + m_tDesc.fDuration + m_tDesc.fEndDelay)
		m_bFinished = true;
}

void CDissolve::Reset()
{
	m_bPlaying = false;
	m_bFinished = false;
	m_fTimer = 0.f;
	m_fProgress = 0.f;
	m_eMode = DISSOLVE_MODE::DISSOLVE_OUT;
}

CDissolve::DISSOLVE_PHASE CDissolve::Get_Phase() const
{
	if (m_bPlaying == false)    
		return DISSOLVE_PHASE::NONE;
	if (m_bFinished)            
		return DISSOLVE_PHASE::DONE;
	if (m_fTimer < m_tDesc.fStartDelay)    
		return DISSOLVE_PHASE::DELAY;
	if (m_fTimer < m_tDesc.fStartDelay + m_tDesc.fDuration)    
		return DISSOLVE_PHASE::RUN;
	
	return DISSOLVE_PHASE::HOLD;
}

void CDissolve::Set_Timing(_float fStartDelay, _float fDuration, _float fEndDelay)
{
	m_tDesc.fStartDelay = fStartDelay; 
	m_tDesc.fDuration = fDuration; 
	m_tDesc.fEndDelay = fEndDelay;
}

CDissolve* CDissolve::Create(void* pArg)
{
	CDissolve* pInstance = new CDissolve();
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CDissolve");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CDissolve::Free()
{
	__super::Free();
}
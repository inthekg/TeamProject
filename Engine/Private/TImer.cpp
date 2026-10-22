#include "TImer.h" 
#include "Engine_Utility.h"
#include "Animator.h"

CTimer::CTimer()
{
}

HRESULT CTimer::Ready_Timer()
{
	QueryPerformanceCounter(&m_FrameTime);			
	QueryPerformanceCounter(&m_LastTime);			
	QueryPerformanceCounter(&m_FixTime);			

	QueryPerformanceFrequency(&m_CpuTick);	

	return S_OK;
}

void CTimer::Update_Timer()
{
	QueryPerformanceCounter(&m_FrameTime);
	if (m_FrameTime.QuadPart - m_FixTime.QuadPart >= m_CpuTick.QuadPart)
	{
		m_FixTime = m_FrameTime;
	}

	m_fTimeDeltaUnscaled = static_cast<float>((m_FrameTime.QuadPart - m_LastTime.QuadPart) / static_cast<_double>(m_CpuTick.QuadPart));
	if (m_fTimeDeltaUnscaled > 0.1f) m_fTimeDeltaUnscaled = 0.1f;


	if (m_fStopRemain > 0.f)
	{
		m_fStopRemain -= m_fTimeDeltaUnscaled;
		if (m_fStopRemain <= 0.f)
		{
			m_fStopRemain = 0.f;
			m_fTimeScale = 1.f;
		}
	}

	//배속,감속용 TimeDelta
	m_fTimeDelta = m_fTimeDeltaUnscaled * m_fTimeScale;

	m_LastTime = m_FrameTime;

}

void CTimer::Set_TimeScale(_float fScale)
{
	m_fTimeScale = fScale;
}

void CTimer::Start()
{
	m_fTimeScale = 1.f;
	m_fStopRemain = 0.f;
	QueryPerformanceCounter(&m_LastTime);
}

void CTimer::Stop()
{
	m_fStopRemain = 0.f;
	m_fTimeScale = 0.f;
}

void CTimer::Slow_Duration(_float fDuration, _float fChangeTime)
{
	if (m_fStopRemain > 0.f) return;
	m_fStopRemain = fDuration;
	
	m_fTimeScale = fChangeTime;
}

CTimer* CTimer::Create()
{
	CTimer* pInstance = new CTimer;

	if (FAILED(pInstance->Ready_Timer()))
	{
		MSG_BOX("Failed to Created : CTimer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTimer::Free()
{
	__super::Free();
}
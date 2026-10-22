#include "Timer_Manager.h"
#include "Timer.h"

CTimer_Manager::CTimer_Manager()
{
}

_float CTimer_Manager::Get_TimeDelta(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return 0.f;

	return pTimer->Get_TimeDelta();
}

_float CTimer_Manager::Get_TimeDeltaUnscaled(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return 0.f;

	return pTimer->Get_TimeDeltaUnscaled();
}

HRESULT CTimer_Manager::Add_Timer(const _wstring& strTimerTag)
{
	if (nullptr != Find_Timer(strTimerTag))
		return E_FAIL;

	CTimer* pTimer = CTimer::Create();
	if (nullptr == pTimer)
		return E_FAIL;

	m_Timers.emplace(strTimerTag, pTimer);

	return S_OK;
}


void CTimer_Manager::Update_Timer(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return;

	pTimer->Update_Timer();
}

void CTimer_Manager::Stop_Timer(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return;

	pTimer->Stop();
}

void CTimer_Manager::Start_Timer(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return;

	pTimer->Start();
}

void CTimer_Manager::Stop_MainTimer()
{
	if (m_pMainTimer) m_pMainTimer->Stop();
}

void CTimer_Manager::Start_MainTimer()
{
	if (m_pMainTimer) m_pMainTimer->Start();
}

void CTimer_Manager::Set_TimeScale(const _wstring& strTimerTag, const float fScale)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return;

	pTimer->Set_TimeScale(fScale);
}

void CTimer_Manager::Reset_Timer(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return;

	pTimer->Set_TimeScale(1.f);
}

HRESULT CTimer_Manager::Register_MainTimer(const _wstring& strTimerTag)
{
	CTimer* pTimer = Find_Timer(strTimerTag);
	if (nullptr == pTimer)
		return E_FAIL;

	m_pMainTimer = pTimer;
	return S_OK;
}

HRESULT CTimer_Manager::Get_MainTimeDeltaUnscaled(_float& fOutUnscaledTime)
{
	if (nullptr == m_pMainTimer)
	{
		MSG_BOX("Timer_Manager : No MainTimer. Register Main Timer");
		return E_FAIL;
	}

	fOutUnscaledTime = m_pMainTimer->Get_TimeDeltaUnscaled();
	return S_OK;
}



void CTimer_Manager::Set_MainTimeScale(_float fScale)
{
	if (nullptr == m_pMainTimer)
		return;

	m_pMainTimer->Set_TimeScale(fScale);
}

void CTimer_Manager::Slow_MainTimer(_float fDuration, _float fChangeTime)
{
	if (nullptr == m_pMainTimer)
		return;

	m_pMainTimer->Slow_Duration(fDuration, fChangeTime);
}

CTimer* CTimer_Manager::Find_Timer(const _wstring& strTimerTag)
{
	auto	iter = m_Timers.find(strTimerTag);

	if (iter == m_Timers.end())
		return nullptr;

	return iter->second;
}

CTimer_Manager* CTimer_Manager::Create()
{
	return new CTimer_Manager();
}

void CTimer_Manager::Free()
{
	__super::Free();

	for (auto& Pair : m_Timers)
		Safe_Release(Pair.second);
	m_Timers.clear();
}

#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CTimer_Manager final : public CBase
{
public:
    static constexpr _float DODGE_TIME_SCALE = 0.15f;

private:
	CTimer_Manager();
	virtual ~CTimer_Manager() = default;

public:
	_float                              Get_TimeDelta(const _wstring& strTimerTag);
	_float                              Get_TimeDeltaUnscaled(const _wstring& strTimerTag);

public:
	HRESULT	                            Add_Timer(const _wstring& strTimerTag);
	void                                Update_Timer(const _wstring& strTimerTag);

public:
	void                                Stop_Timer(const _wstring& strTimerTag);
	void                                Start_Timer(const _wstring& strTimerTag);


	void                                Set_TimeScale(const _wstring& strTimerTag, const float fScale);
	void                                Reset_Timer(const _wstring& strTimerTag);

public:
	//Main Timer 설정 
	HRESULT                             Register_MainTimer(const _wstring& strTimerTag);
	
	HRESULT                             Get_MainTimeDeltaUnscaled(_float& fOutUnscaledTime);

	void                                Set_MainTimeScale(_float fScale);
	void								Slow_MainTimer(_float fDuration, _float fChangeTime = 0.f);

	void								Stop_MainTimer();
	void								Start_MainTimer();

private:
	class CTimer*                       Find_Timer(const _wstring& strTimerTag);

private:
	map<_wstring, class CTimer*>		m_Timers;
	class CTimer*						m_pMainTimer = { nullptr };


public:
	static CTimer_Manager*              Create();
	virtual void		                Free() override;
};

NS_END
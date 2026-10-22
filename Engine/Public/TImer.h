#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CTimer final : public CBase
{
private:
	CTimer();
	virtual ~CTimer() = default;

public:
	_float Get_TimeDelta() const 
	{
		return m_fTimeDelta;
	}

	_float Get_TimeDeltaUnscaled() const
	{
		return m_fTimeDeltaUnscaled;
	}

public:
	HRESULT				Ready_Timer();
	void				Update_Timer();

	void				Set_TimeScale(_float fScale);
	void				Start();
	void				Stop();
	void				Slow_Duration(_float fDuration, _float fChangeTime = 0.f);

private:
	LARGE_INTEGER		m_FrameTime = {};
	LARGE_INTEGER		m_FixTime = {};
	LARGE_INTEGER		m_LastTime = {};
	LARGE_INTEGER		m_CpuTick = {};

	_float				m_fTimeDelta = {};
	_float				m_fTimeDeltaUnscaled = {};
	_float				m_fTimeScale = {1.f};

	_float				m_fStopRemain = {};
public:
	static CTimer*		Create();
	virtual void		Free();

};

NS_END

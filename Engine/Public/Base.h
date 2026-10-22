#pragma once

#include "Engine_Defines.h"
#include <atomic>

NS_BEGIN(Engine)

class ENGINE_DLL CBase abstract
{
protected:
	CBase();
	CBase(const CBase&);
	virtual ~CBase() = default;

public:
	_uint AddRef();
	//TODO : CBase 가상으로 수정.PoolObject만 재정의하기 
	virtual _uint Release();

protected:
	atomic<_uint> m_iRefCnt = {};

public:
	virtual void Free();
	//TODO : 프레임워크 수정사항 - CBase. Clone 최상위로 옮김 
	virtual CBase* Clone(void* pArg) { return nullptr; }

};

NS_END
#pragma once
#include "BT_Decorator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Repeat final : public CBT_Decorator
{
private :
	CBT_Repeat(const _wstring& strName, CBT_Node* pChild, _int iRepeatCount);
	virtual ~CBT_Repeat() = default;

public :
	virtual Status Tick() override;
	virtual void Reset() override;
	virtual void Abort() override;

private :
	
	_int m_iRepeatCount = { -1 }; // -1 무한 루프 ( 목표 반복 횟수 )
	_int m_iCurrentCount = { 0 }; // 현재까지 성공한 횟수

public :
	static CBT_Repeat* Create(const _wstring& strName, CBT_Node* pChild = nullptr, _int iRepeatCount = -1);
	virtual void Free() override;
};

NS_END
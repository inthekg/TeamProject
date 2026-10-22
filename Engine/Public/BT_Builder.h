#pragma once
#include "Base.h"

/* 베이스/공용 */
#include "BT_Defines.h"
#include "BT_Node.h"
#include "BT_Composite.h"
#include "BT_Decorator.h"

/* 컨테이너(Composite) */
#include "BT_Selector.h"
#include "BT_Sequence.h"
#include "BT_ReactiveSelector.h"
#include "BT_ReactiveSequence.h"
#include "BT_Parallel.h"
#include "BT_RandomSelector.h"

/* 데코레이터(Decorator) */
#include "BT_Inverter.h"
#include "BT_Repeat.h"
#include "BT_Cooldown.h"
#include "BT_ForceSuccess.h"
#include "BT_ForceFailure.h"
#include "BT_Service.h"

/* 리프(Leaf) */
#include "BT_Task.h"
#include "BT_Condition.h"
#include "BT_Wait.h"

NS_BEGIN(Engine)


class ENGINE_DLL CBT_Builder final : public CBase
{
public : 
	using TaskFunc		= CBT_Task::TaskFunc;
	using ConditionFunc = CBT_Condition::ConditionFunc;
	using ServiceFunc	= CBT_Service::ServiceFunc;

private:
	CBT_Builder();
	virtual ~CBT_Builder() = default;

public :
	CBT_Builder&	Selector(const _wstring& strName);
	CBT_Builder&	Sequence(const _wstring& strName);
	CBT_Builder&	ReactiveSelector(const _wstring& strName);
	CBT_Builder&	ReactiveSequence(const _wstring& strName);
	CBT_Builder&	Parallel(const _wstring& strName,
		CBT_Parallel::Parallel_Policy eSuccess = CBT_Parallel::Parallel_Policy::Require_One,
		CBT_Parallel::Parallel_Policy eFailure = CBT_Parallel::Parallel_Policy::Require_One);
	CBT_Builder&	RandomSelector(const _wstring& strName);

	CBT_Builder&	Inverter(const _wstring& strName);
	CBT_Builder&	Repeat(const _wstring& strName, _int iCount = -1);
	CBT_Builder&	Cooldown(const _wstring& strName, _float fSeconds);
	CBT_Builder&	ForceSuccess(const _wstring& strName);
	CBT_Builder&	ForceFailure(const _wstring& strName);
	CBT_Builder&	Service(const _wstring& strName, ServiceFunc func, _float fInterval);

	CBT_Builder&	Task(const _wstring& strName, TaskFunc func);
	CBT_Builder&	Condition(const _wstring& strName, ConditionFunc func);
	CBT_Builder&	Wait(const _wstring& strName, _float fSeconds);

	CBT_Builder&	End();
	CBT_Node*		Build() { return m_pRoot; }

private :
	void			Attach(CBT_Node* pNode);
	void			Push(CBT_Node* pNode);
	void			Leaf(CBT_Node* pNode);


private :
	vector<CBT_Node*>	m_Stack;
	CBT_Node* m_pRoot = { nullptr };

public :
	static CBT_Builder* Create();
	virtual void Free() override;
};

NS_END
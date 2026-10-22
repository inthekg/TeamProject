#pragma once
#include "Base.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)
class CStatusComponent;
class CStatComponent;
class CInputComponent;

struct StateContext
{
	CStatusComponent*	pStatusCom = {};
	CStatComponent*		pStatCom = {};
	CInputComponent*	pInputCom = {};
};

using Guard = function<bool(StateContext&)>;
struct Transition
{
	_uint				iPriority = {};
	PlayerActionID		ActionID = {};
	Guard				Guard_Func = {};

	Transition(int p, PlayerActionID t, Guard g)
		: iPriority(p), ActionID(t), Guard_Func(move(g)) {
	}
};

inline void Sort_TransitionList(vector<Transition>& transitionList)
{
	sort(transitionList.begin(), transitionList.end(), [](const Transition& a, const Transition& b) 
	{
		return a.iPriority > b.iPriority;
	});
}

class CState abstract : public CBase
{
public:
	virtual ~CState() = default;
	virtual void Begin(StateContext& Context) {};
	virtual void Update(StateContext& Context, _float fTimeDelta) {};
	virtual void End(StateContext& Context) {};
	//virtual void OnNotify(NotifyEvent eNotify) {};
};

NS_END
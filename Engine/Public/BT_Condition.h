#pragma once
#include "BT_Node.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Condition final : public CBT_Node
{
public :
	using ConditionFunc = std::function<_bool(class CBT_BlackBoard*)>;

private :
	CBT_Condition(const _wstring& strName);
	virtual ~CBT_Condition() = default;

public:
	virtual Status OnUpdate() override;

private:
	ConditionFunc m_func;

public:
	static CBT_Condition* Create(const _wstring& strName, ConditionFunc func);
	virtual void Free() override;
};

NS_END

#pragma once
#include "BT_Node.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Task final : public CBT_Node
{
public :
	using TaskFunc = std::function<Status(class CBT_BlackBoard*)>;

private :
	CBT_Task(const _wstring& strName);
	virtual ~CBT_Task() = default;

private :
	virtual Status OnUpdate() override;

private :
	TaskFunc m_func;

public :
	static CBT_Task* Create(const _wstring& strName, TaskFunc func);
	virtual void Free() override;
};

NS_END
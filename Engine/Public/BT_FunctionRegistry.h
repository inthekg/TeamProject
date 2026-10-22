#pragma once
#include "Base.h"
#include "BT_Node.h"            // CBT_Node::Status
#include "BT_BlackBoard.h"
#include "BT_NodeModel.h"       // BT_PARAMS

NS_BEGIN(Engine)

class ENGINE_DLL CBT_FunctionRegistry final : public CBase
{
public:	
	using TaskFunc      = std::function<CBT_Node::Status(CBT_BlackBoard*, const BT_PARAMS&)>;
	using ConditionFunc = std::function<_bool(CBT_BlackBoard*, const BT_PARAMS&)>;
	using ServiceFunc   = std::function<void(CBT_BlackBoard*, const BT_PARAMS&)>;
	
	static std::string Param(const BT_PARAMS& params, const std::string& strKey, const std::string& strDef = "");

private:
	CBT_FunctionRegistry() = default;
	virtual ~CBT_FunctionRegistry() = default;

public:
	
	void Register_Task(const string& strKey, TaskFunc func);
	void Register_Condition(const string& strKey, ConditionFunc func);
	void Register_Service(const string& strKey, ServiceFunc func);

	_bool Has_Task(const string& strKey) const;
	_bool Has_Condition(const string& strKey) const;
	_bool Has_Service(const string& strKey) const;

	TaskFunc      Find_Task(const string& strKey) const;
	ConditionFunc Find_Condition(const string& strKey) const;
	ServiceFunc   Find_Service(const string& strKey) const;

	vector<string> Task_Keys() const;
	vector<string> Condition_Keys() const;
	vector<string> Service_Keys() const;

private:
	unordered_map<string, TaskFunc>      m_Tasks;
	unordered_map<string, ConditionFunc> m_Conditions;
	unordered_map<string, ServiceFunc>   m_Services;

public:
	static CBT_FunctionRegistry* Create();
	virtual void Free() override;
};

NS_END

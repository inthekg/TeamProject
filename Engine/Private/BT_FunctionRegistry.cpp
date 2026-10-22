#include "BT_FunctionRegistry.h"

using Status = CBT_Node::Status;

std::string CBT_FunctionRegistry::Param(const BT_PARAMS& params, const std::string& strKey, const std::string& strDef)
{
	auto it = params.find(strKey);
	return (it != params.end()) ? it->second : strDef;
}

void CBT_FunctionRegistry::Register_Task(const string& strKey, TaskFunc func)
{
	if (strKey.empty() || !func)
		return;

	m_Tasks[strKey] = func;
}

void CBT_FunctionRegistry::Register_Condition(const string& strKey, ConditionFunc func)
{
	if (strKey.empty() || !func)
		return;

	m_Conditions[strKey] = func;
}

void CBT_FunctionRegistry::Register_Service(const string& strKey, ServiceFunc func)
{
	if (strKey.empty() || !func)
		return;

	m_Services[strKey] = func;
}

_bool CBT_FunctionRegistry::Has_Task(const string& strKey) const
{
	return m_Tasks.find(strKey) != m_Tasks.end();
}

_bool CBT_FunctionRegistry::Has_Condition(const string& strKey) const
{
	return m_Conditions.find(strKey) != m_Conditions.end();
}

_bool CBT_FunctionRegistry::Has_Service(const string& strKey) const
{
	return m_Services.find(strKey) != m_Services.end();
}

CBT_FunctionRegistry::TaskFunc CBT_FunctionRegistry::Find_Task(const string& strKey) const
{
	auto it = m_Tasks.find(strKey);
	return (it != m_Tasks.end()) ? it->second : TaskFunc{};
}

CBT_FunctionRegistry::ConditionFunc CBT_FunctionRegistry::Find_Condition(const string& strKey) const
{
	auto it = m_Conditions.find(strKey);
	return (it != m_Conditions.end()) ? it->second : ConditionFunc{};
}

CBT_FunctionRegistry::ServiceFunc CBT_FunctionRegistry::Find_Service(const string& strKey) const
{
	auto it = m_Services.find(strKey);
	return (it != m_Services.end()) ? it->second : ServiceFunc{};
}

vector<string> CBT_FunctionRegistry::Task_Keys() const
{
	vector<string> Keys;
	Keys.reserve(m_Tasks.size());
	for (const auto& Pair : m_Tasks)
		Keys.push_back(Pair.first);
	std::sort(Keys.begin(), Keys.end());
	return Keys;
}

vector<string> CBT_FunctionRegistry::Condition_Keys() const
{
	vector<string> Keys;
	Keys.reserve(m_Conditions.size());
	for (const auto& Pair : m_Conditions)
		Keys.push_back(Pair.first);
	std::sort(Keys.begin(), Keys.end());
	return Keys;
}

vector<string> CBT_FunctionRegistry::Service_Keys() const
{
	vector<string> Keys;
	Keys.reserve(m_Services.size());
	for (const auto& Pair : m_Services)
		Keys.push_back(Pair.first);
	std::sort(Keys.begin(), Keys.end());
	return Keys;
}


CBT_FunctionRegistry* CBT_FunctionRegistry::Create()
{
	CBT_FunctionRegistry* pInstance = new CBT_FunctionRegistry();

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_FunctionRegistry");
		Safe_Release(pInstance);
		return nullptr;
	}

	return pInstance;
}

void CBT_FunctionRegistry::Free()
{
	m_Tasks.clear();
	m_Conditions.clear();
	m_Services.clear();

	__super::Free();
}
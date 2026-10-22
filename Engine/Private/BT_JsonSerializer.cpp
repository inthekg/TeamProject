#include "BT_JsonSerializer.h"

static constexpr int BTJ_VERSION = 1;

static const json* ValidateDocAndFindRoot(const json& jDoc)
{	
	if (false == jDoc.is_object())
		return nullptr;
	
	auto itVersion = jDoc.find("version");
	if (itVersion == jDoc.end() || false == itVersion->is_number_integer()
		|| itVersion->get<int>() != BTJ_VERSION)
		return nullptr;
	
	auto itRoot = jDoc.find("root");
	if (itRoot == jDoc.end())
		return nullptr;

	return &(*itRoot);
}

static const char* Policy_ToName(_int iPolicy)
{
	return (iPolicy == 1) ? "RequireAll" : "RequireOne";
}

static _int Policy_FromName(const string& strName)
{
	return (strName == "RequireAll") ? 1 : 0;
}

_bool CBT_JsonSerializer::Save(const BT_NODE_MODEL* pRoot, const string& strPath)
{
	if (nullptr == pRoot)
		return false;

	try
	{
		json jDoc = json::object();
		jDoc["version"] = BTJ_VERSION;

		json jRoot;
		Write(pRoot, jRoot);
		jDoc["root"] = std::move(jRoot);
		
		std::ofstream ofs(strPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (false == ofs.is_open())
			return false;

		const string strDump = jDoc.dump(2);
		ofs.write(strDump.data(), static_cast<std::streamsize>(strDump.size()));
		ofs.close();

		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

_bool CBT_JsonSerializer::Load(const string& strPath, BT_NODE_MODEL** ppOutRoot)
{
	if (nullptr == ppOutRoot)
		return false;

	*ppOutRoot = nullptr;

	try
	{
		std::ifstream ifs(strPath, std::ios::in | std::ios::binary);
		if (false == ifs.is_open())
			return false;

		json jDoc = json::parse(ifs, nullptr, true, true);
		ifs.close();
		
		const json* pjRoot = ValidateDocAndFindRoot(jDoc);
		if (nullptr == pjRoot)
			return false;

		BT_NODE_MODEL* pRoot = Read(*pjRoot);
		if (nullptr == pRoot)
			return false;

		*ppOutRoot = pRoot;
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

string CBT_JsonSerializer::ToJsonString(const BT_NODE_MODEL* pRoot)
{
	if (nullptr == pRoot)
		return string();

	try
	{
		json jDoc = json::object();
		jDoc["version"] = BTJ_VERSION;

		json jRoot;
		Write(pRoot, jRoot);
		jDoc["root"] = std::move(jRoot);

		return jDoc.dump(2);
	}
	catch (const std::exception&)
	{
		return string();
	}
}

BT_NODE_MODEL* CBT_JsonSerializer::FromJsonString(const string& strJson)
{
	try
	{
		json jDoc = json::parse(strJson, nullptr, true, true);
		
		const json* pjRoot = ValidateDocAndFindRoot(jDoc);
		if (nullptr == pjRoot)
			return nullptr;

		return Read(*pjRoot);
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}


void CBT_JsonSerializer::Write(const BT_NODE_MODEL* pNode, json& jOut)
{
	if (nullptr == pNode)
	{
		jOut = nullptr;
		return;
	}

	jOut = json::object();
	jOut["type"] = BTNode_TypeName(pNode->eType);
	jOut["name"] = pNode->strName;
	
	if (false == pNode->strFuncKey.empty())
		jOut["funcKey"] = pNode->strFuncKey;
	
	if (false == pNode->Params.empty())
	{
		json jParams = json::object();
		for (const auto& kv : pNode->Params)
			jParams[kv.first] = kv.second;
		jOut["params"] = std::move(jParams);
	}

	jOut["repeatCount"] = pNode->iRepeatCount;
	jOut["seconds"] = pNode->fSeconds;
	jOut["interval"] = pNode->fInterval;
	jOut["successPolicy"] = Policy_ToName(pNode->iSuccessPolicy);
	jOut["failurePolicy"] = Policy_ToName(pNode->iFailurePolicy);

	if (false == pNode->Weights.empty())
		jOut["weights"] = pNode->Weights;
	
	if (false == pNode->Children.empty())
	{
		json jChildren = json::array();
		for (const BT_NODE_MODEL* pChild : pNode->Children)
		{
			json jChild;
			Write(pChild, jChild);
			jChildren.push_back(std::move(jChild));
		}
		jOut["children"] = std::move(jChildren);
	}
}


BT_NODE_MODEL* CBT_JsonSerializer::Read(const json& jNode)
{
	if (false == jNode.is_object())
		return nullptr;
	
	auto itType = jNode.find("type");
	if (itType == jNode.end() || false == itType->is_string())
		return nullptr;

	BT_NodeType eType = BTNode_TypeFromName(itType->get<string>());
	if (BT_NodeType::None == eType)
		return nullptr;

	BT_NODE_MODEL* pNode = new BT_NODE_MODEL();
	pNode->eType = eType;

	{
		auto it = jNode.find("name");
		if (it != jNode.end() && it->is_string())
			pNode->strName = it->get<string>();
	}

	{
		auto it = jNode.find("funcKey");
		if (it != jNode.end() && it->is_string())
			pNode->strFuncKey = it->get<string>();
	}
	
	{
		auto it = jNode.find("params");
		if (it != jNode.end() && it->is_object())
		{
			for (auto kv = it->begin(); kv != it->end(); ++kv)
			{
				if (kv.value().is_string())
					pNode->Params[kv.key()] = kv.value().get<string>();
			}
		}
	}

	{
		auto it = jNode.find("repeatCount");
		if (it != jNode.end() && it->is_number_integer())
			pNode->iRepeatCount = it->get<_int>();
	}

	{
		auto it = jNode.find("seconds");
		if (it != jNode.end() && it->is_number())
			pNode->fSeconds = it->get<_float>();
	}

	{
		auto it = jNode.find("interval");
		if (it != jNode.end() && it->is_number())
			pNode->fInterval = it->get<_float>();
	}

	{
		auto it = jNode.find("successPolicy");
		if (it != jNode.end() && it->is_string())
			pNode->iSuccessPolicy = Policy_FromName(it->get<string>());
	}

	{
		auto it = jNode.find("failurePolicy");
		if (it != jNode.end() && it->is_string())
			pNode->iFailurePolicy = Policy_FromName(it->get<string>());
	}

	{
		auto it = jNode.find("weights");
		if (it != jNode.end() && it->is_array())
		{
			pNode->Weights.clear();
			pNode->Weights.reserve(it->size());
			for (const json& jW : *it)
			{
				if (jW.is_number())
					pNode->Weights.push_back(jW.get<_float>());
			}
		}
	}

	{
		auto it = jNode.find("children");
		if (it != jNode.end() && it->is_array())
		{
			for (const json& jChild : *it)
			{
				BT_NODE_MODEL* pChild = Read(jChild);
				if (nullptr == pChild)
				{
					delete pNode;
					return nullptr;
				}
				pNode->Children.push_back(pChild);
			}
		}
	}
	

	return pNode;
}

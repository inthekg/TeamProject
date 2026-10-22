#pragma once
#include "BT_NodeModel.h"
#include "BT_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_JsonSerializer final
{
private :
	CBT_JsonSerializer() = delete;
	~CBT_JsonSerializer() = delete;
	CBT_JsonSerializer(const CBT_JsonSerializer&) = delete;
	CBT_JsonSerializer& operator=(const CBT_JsonSerializer&) = delete;

public:	
	static _bool			Save(const BT_NODE_MODEL* pRoot, const string& strPath);	
	static _bool			Load(const string& strPath, BT_NODE_MODEL** ppOutRoot);	
	static string			ToJsonString(const BT_NODE_MODEL* pRoot);	
	static BT_NODE_MODEL*	FromJsonString(const string& strJson);

private:	
	static void				Write(const BT_NODE_MODEL* pNode, json& jOut);	
	static BT_NODE_MODEL*	Read(const json& jNode);
};

NS_END
#pragma once
#include "BT_Defines.h"


NS_BEGIN(Engine)

class CBT_Node;
struct BT_NODE_MODEL;
class  CBT_FunctionRegistry;

class ENGINE_DLL CBT_TreeCompiler final
{
private :
	CBT_TreeCompiler() = delete;
	~CBT_TreeCompiler() = delete;
	CBT_TreeCompiler(const CBT_TreeCompiler&) = delete;
	CBT_TreeCompiler& operator = (const CBT_TreeCompiler&) = delete;

public :
	static CBT_Node* Compile(const BT_NODE_MODEL* pModel, CBT_FunctionRegistry* pRegistry);
};


NS_END

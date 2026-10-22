#pragma once
#include "Base.h"
#include "BT_Defines.h"

NS_BEGIN(Engine)

using BT_PARAMS = std::unordered_map<std::string, std::string>;

struct ENGINE_DLL BT_NODE_MODEL
{
	BT_NodeType				eType = BT_NodeType::None;
	string					strName;
	string					strFuncKey;
	int						iRepeatCount = -1;
	float					fSeconds = 0.f;
	float					fInterval = 0.f;
	int						iSuccessPolicy = 0;
	int						iFailurePolicy = 0;
	vector<float>			Weights;
	BT_PARAMS				Params;
	vector<BT_NODE_MODEL*>	Children;

	~BT_NODE_MODEL();

	string			Get_Param(const string& key, const string& def = "") const;

	BT_NODE_MODEL*	Add_Child(BT_NodeType eType, const string& strName);
	_bool           Add_Child(BT_NODE_MODEL* pChild);	
	void            Remove_Child(BT_NODE_MODEL* pChild);	
	_bool           Can_AddChild() const;
};

class ENGINE_DLL CBT_TreeModel final : public CBase
{
private :
	CBT_TreeModel();
	virtual ~CBT_TreeModel() = default;

public:
	BT_NODE_MODEL*		Get_Root() const { return m_pRoot; }
	void				Set_Root(BT_NODE_MODEL* pRoot);	

public :

	BT_NODE_MODEL*		New_Root(BT_NodeType eType, const string& strName);
	void				Clear();

private:
	BT_NODE_MODEL* m_pRoot = { nullptr };

public:
	static CBT_TreeModel* Create();
	virtual void          Free() override;
};

NS_END
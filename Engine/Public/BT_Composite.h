#pragma once
#include "BT_Node.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Composite abstract : public CBT_Node
{
protected :
	CBT_Composite(const _wstring& strName);
	virtual ~CBT_Composite() = default;

public :
	void					Add_Child(CBT_Node* pChild);
	virtual void			Attach_Child(CBT_Node* pChild) { Add_Child(pChild); }
	virtual void			Set_BlackBoard(class CBT_BlackBoard* pBlackBoard) override;	
	virtual void			Reset() override;
	virtual void			Abort() override;
	virtual const CBT_Node* Get_RunningLeaf() const override;

protected :
	vector<CBT_Node*>		m_Childrens;
	_uint					m_iCurrentIndex;

public :
	virtual void Free() override;
};

NS_END
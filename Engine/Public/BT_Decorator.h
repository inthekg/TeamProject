#pragma once
#include "BT_Node.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Decorator abstract : public CBT_Node
{
protected :
	CBT_Decorator(const _wstring& strName, CBT_Node* pChild = nullptr);
	virtual ~CBT_Decorator() = default;

public :
	void					Set_Child(CBT_Node* pChild);
	virtual void			Attach_Child(CBT_Node* pChild) { Set_Child(pChild); }
	virtual void			Set_BlackBoard(class CBT_BlackBoard* pBlackBoard) override;
	virtual void			Reset() override;
	virtual void			Abort() override;
	virtual const CBT_Node* Get_RunningLeaf() const override;	

protected :
	CBT_Node* m_pChild = { nullptr };

public :
	virtual void Free() override;
};

NS_END
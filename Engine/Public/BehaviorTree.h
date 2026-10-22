#pragma once
#include "Base.h"
#include "BT_Node.h"
#include "BT_BlackBoard.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBehaviorTree final : public CBase
{
private : 
	CBehaviorTree();
	virtual ~CBehaviorTree() = default;

public :
	CBT_Node::Status		Get_LastStatus()	const { return m_eLastStatus; }
	CBT_BlackBoard*			Get_BlackBoard()	const { return m_pBlackBoard; }
	_bool					Is_AutoReset()		const { return m_bAutoReset; }

	void					Set_Root(class CBT_Node* pRoot);
	void					Set_BlackBoard(CBT_BlackBoard* pBlackBoard);
	void					Set_AutoReset(_bool bAutoReset) { m_bAutoReset = bAutoReset; }
public :
	//HRESULT					Initialize();
	void					Update();
	void					Reset();
	const _wstring			Get_RunningLeafName() const;

private :
	CBT_Node*				m_pRoot = { nullptr };
	CBT_BlackBoard*			m_pBlackBoard = { nullptr };
	CBT_Node::Status		m_eLastStatus = { CBT_Node::Status::Failure };
	_bool					m_bAutoReset = { true };
	_bool					m_bOwnsBlackBoard = { true };

public :
	static CBehaviorTree* Create(CBT_BlackBoard* pShared = nullptr);
	virtual void Free() override;

};

NS_END
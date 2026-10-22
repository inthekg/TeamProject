#pragma once
#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Node abstract : public CBase
{
public :
	enum class Status
	{
		Success,
		Failure,
		Running
	};	

protected :
	CBT_Node(const _wstring& strName);
	virtual ~CBT_Node() = default;

public:
	Status					Get_Status() const { return m_eStatus; }
	const _wstring&			Get_Name() const { return m_strName; }
	class CBT_BlackBoard*	Get_BlackBoard() const { return m_pBlackBoard; }
	virtual void			Set_BlackBoard(class CBT_BlackBoard* pBlackBoard) { m_pBlackBoard = pBlackBoard; }
	virtual void			Attach_Child(CBT_Node* pChild) {}
public :
	virtual Status			Tick();
	virtual void			Reset();
	virtual void			Abort();
	virtual const CBT_Node* Get_RunningLeaf() const;

protected :
	virtual void			OnEnter() {}
	virtual Status			OnUpdate() { return Status::Failure; }
	virtual void			OnExit(Status eFinal) {}
	

protected :
	Status			m_eStatus = { Status::Failure };
	_wstring		m_strName = { };

protected :
	class CBT_BlackBoard* m_pBlackBoard = { nullptr };

public : 
	virtual void Free() override;

};

NS_END
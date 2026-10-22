#pragma once
#include "BT_Composite.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBT_Parallel final : public CBT_Composite
{
public :
	enum class Parallel_Policy
	{
		Require_One,
		Require_All
	};

private :
	CBT_Parallel(const _wstring& strName, Parallel_Policy eSuccess, Parallel_Policy eFailure);
	virtual ~CBT_Parallel() = default;

public :
	virtual Status	Tick() override;
	virtual void	Reset() override;
	virtual void	Abort() override;

private :
	Parallel_Policy m_eSuccess = { Parallel_Policy::Require_One };
	Parallel_Policy m_eFailure = { Parallel_Policy::Require_One };
	vector<Status>	m_ChildStatus;

public :
	static CBT_Parallel* Create(const _wstring& strName,
		Parallel_Policy eSuccess = Parallel_Policy::Require_One,
		Parallel_Policy eFailure = Parallel_Policy::Require_One);
	virtual void Free() override;
};


NS_END
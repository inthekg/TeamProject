#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CNavigation;

class ENGINE_DLL CNavigation_Manager final : public CBase
{
private:
	CNavigation_Manager();
	virtual ~CNavigation_Manager() = default;

public:
	HRESULT Load_Navigation(const _wstring& strChapterKey, _uint iSectorID, const _wstring& strNavigationPath, const _wstring& strNeighborPath);
	CNavigation* Get_Navigation(const _wstring& strChapterKey, _uint iSectorID) const;
	void Clear_Navigations(const _wstring& strChapterKey);
	

private:
	_wstring Make_NavigationKey(const _wstring& strChapterKey, _uint iSectorID) const ;
	void Clear_AllNavigations();

private:
	unordered_map<_wstring, CNavigation*> m_Navigations = {};

public:
	static CNavigation_Manager* Create();
	virtual void Free() override;
};

NS_END
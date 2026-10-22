#include "Navigation_Manager.h"
#include "Navigation.h"

CNavigation_Manager::CNavigation_Manager()
{
}

HRESULT CNavigation_Manager::Load_Navigation(const _wstring& strChapterKey, _uint iSectorID, const _wstring& strNavigationPath, const _wstring& strNeighborPath)
{
	if (strChapterKey.empty() || UINT_MAX == iSectorID || strNavigationPath.empty() || strNeighborPath.empty())
		return E_FAIL;

	const _wstring strNavigationKey = Make_NavigationKey(strChapterKey, iSectorID);

	if (m_Navigations.end() != m_Navigations.find(strNavigationKey))
		return S_OK;

	CNavigation* pNavigation = CNavigation::Create(strNavigationPath, strNeighborPath);

	if (nullptr == pNavigation)
		return E_FAIL;

	// Engine 네비 매니저가 실제 네비 리소스를 캐싱한다.
	m_Navigations.emplace(strNavigationKey, pNavigation);

	return S_OK;
}


CNavigation* CNavigation_Manager::Get_Navigation(const _wstring& strChapterKey, _uint iSectorID) const
{
	const _wstring strNavigationKey = Make_NavigationKey(strChapterKey, iSectorID);

	auto Iter = m_Navigations.find(strNavigationKey);

	if (m_Navigations.end() == Iter)
		return nullptr;

	return Iter->second;
}

void CNavigation_Manager::Clear_Navigations(const _wstring& strChapterKey)
{
	if (strChapterKey.empty())
		return;

	vector<_wstring> RemoveKeys = {};

	for (const auto& Pair : m_Navigations)
	{
		// ChapterKey prefix가 같은 네비만 챕터 단위로 제거한다.
		if (0 == Pair.first.find(strChapterKey + L"#"))
			RemoveKeys.push_back(Pair.first);
	}

	for (const _wstring& strKey : RemoveKeys)
	{
		Safe_Release(m_Navigations[strKey]);
		m_Navigations.erase(strKey);
	}
}

_wstring CNavigation_Manager::Make_NavigationKey(const _wstring& strChapterKey, _uint iSectorID) const
{
	return strChapterKey + L"#" + to_wstring(iSectorID);
}

void CNavigation_Manager::Clear_AllNavigations()
{
	for (auto& Pair : m_Navigations)
		Safe_Release(Pair.second);

	m_Navigations.clear();
}

CNavigation_Manager* CNavigation_Manager::Create()
{
	CNavigation_Manager* pInstance = new CNavigation_Manager();

	if (nullptr == pInstance)
		MSG_BOX("Failed to Created : CNavigation_Manager");

	return pInstance;
}

void CNavigation_Manager::Free()
{
	Clear_AllNavigations();
	__super::Free();
}
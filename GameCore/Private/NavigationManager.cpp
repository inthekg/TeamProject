#include "NavigationManager.h"
#include "GameInstance.h"
#include "Navigation.h"

CNavigationManager::CNavigationManager()
{
}

HRESULT CNavigationManager::Load_ChapterNavigation(const _wstring& strChapterPath, const CHAPTER_DESC& ChapterDesc)
{
	Clear_Navigations();

	Engine::CGameInstance* pGameInstance = Engine::CGameInstance::Get_Instance();

	if (nullptr == pGameInstance)
		return E_FAIL;

	const _wstring strChapterNavigationKey = Make_ChapterNavigationKey(strChapterPath);

	for (const auto& Pair : ChapterDesc.Sectors)
	{
		const SECTOR_DESC& Sector = Pair.second;
		Engine::CNavigation* pNavigation = pGameInstance->Get_Navigation(strChapterNavigationKey, Sector.ID);

		if (nullptr == pNavigation)
			continue;

		// GameCore 네비 매니저는 Engine에 캐싱된 네비를 섹터 ID 기준으로 참조만 보관한다.
		Safe_AddRef(pNavigation);
		m_Navigations.emplace(Sector.ID, pNavigation);
	}

	return S_OK;
}

Engine::CNavigation* CNavigationManager::Find_Navigation(_uint iSectorID) const
{
	auto Iter = m_Navigations.find(iSectorID);

	if (m_Navigations.end() == Iter)
		return nullptr;

	return Iter->second;
}

_wstring CNavigationManager::Make_ChapterNavigationKey(const _wstring& strChapterPath) 
{
	const size_t iSlashPosition = strChapterPath.find_last_of(L"\\/");
	const size_t iFileNamePosition = (_wstring::npos == iSlashPosition) ? 0 : iSlashPosition + 1;
	const size_t iExtensionPosition = strChapterPath.find(L'.', iFileNamePosition);

	if (_wstring::npos == iExtensionPosition)
		return strChapterPath.substr(iFileNamePosition);

	return strChapterPath.substr(iFileNamePosition, iExtensionPosition - iFileNamePosition);
}

void CNavigationManager::Clear_Navigations()
{
	for (auto& Pair : m_Navigations)
		Safe_Release(Pair.second);

	m_Navigations.clear();
}

CNavigationManager* CNavigationManager::Create()
{
	CNavigationManager* pInstance = new CNavigationManager();

	if (nullptr == pInstance)
		MSG_BOX("Failed to Created : CNavigationManager");

	return pInstance;
}

void CNavigationManager::Free()
{
	Clear_Navigations();
	__super::Free();
}
#pragma once

#include "Core_Defines.h"
#include "Base.h"
#include "ChapterMapData.h"

NS_BEGIN(Engine)
class CNavigation;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CNavigationManager final : public CBase
{
private:
	CNavigationManager();
	virtual ~CNavigationManager() = default;

public:
	HRESULT Load_ChapterNavigation(const _wstring& strChapterPath, const CHAPTER_DESC& ChapterDesc);
	Engine::CNavigation* Find_Navigation(_uint iSectorID) const;

	static _wstring Make_ChapterNavigationKey(const _wstring& strChapterPath);

private:

	void Clear_Navigations();

private:
	unordered_map<_uint, Engine::CNavigation*> m_Navigations = {};

public:
	static CNavigationManager* Create();
	virtual void Free() override;
};

NS_END
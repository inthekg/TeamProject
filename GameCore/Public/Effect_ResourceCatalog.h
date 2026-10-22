#pragma once

#include "Base.h"

NS_BEGIN(GameCore)

// Resources\\Effects 안의 원본 JSON과 사용자가 지정한 런타임 키를 연결하는 저장 단위다.
typedef struct tagEffectResourceCatalogEntry
{
	string strKey = {};
	_wstring strRelativeFilePath = {};
}EFFECT_RESOURCE_CATALOG_ENTRY;

class GAMECORE_DLL CEffect_ResourceCatalog final
{
public:
	static HRESULT Load(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pOutEntries, string* pOutErrorMessage = nullptr);
	static HRESULT Save(const _wstring& strEffectsDirectory, const vector<EFFECT_RESOURCE_CATALOG_ENTRY>& Entries, string* pOutErrorMessage = nullptr);
	static HRESULT Rename_Key(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pEntries, const string& strOldKey, const string& strNewKey, string* pOutErrorMessage = nullptr);
	static HRESULT Delete_Key(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pEntries, const string& strKey, string* pOutErrorMessage = nullptr);
	static void Collect_SourceFiles(const _wstring& strEffectsDirectory, vector<_wstring>* pOutRelativeFilePaths);
	static const EFFECT_RESOURCE_CATALOG_ENTRY* Find_ByKey(const vector<EFFECT_RESOURCE_CATALOG_ENTRY>& Entries, const string& strKey);
	static _wstring Resolve_SourcePath(const _wstring& strEffectsDirectory, const _wstring& strRelativeFilePath);
	static _bool Is_ValidRelativeFilePath(const _wstring& strRelativeFilePath);

public:
	static const _wstring& Get_LibraryFileName();
};

NS_END

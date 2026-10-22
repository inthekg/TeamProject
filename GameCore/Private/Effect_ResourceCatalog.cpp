#include "Effect_ResourceCatalog.h"
#include "Engine_Utility.h"

#include <algorithm>
#include <fstream>

static _wstring EffectCatalog_NormalizeRelativePath(const _wstring& strRelativeFilePath)
{
	_wstring strNormalizedPath = strRelativeFilePath;
	replace(strNormalizedPath.begin(), strNormalizedPath.end(), L'/', L'\\');

	while (false == strNormalizedPath.empty() && L'\\' == strNormalizedPath.front())
		strNormalizedPath.erase(strNormalizedPath.begin());

	return strNormalizedPath;
}

static _bool EffectCatalog_IsJsonFile(const _wstring& strFileName)
{
	static const _wstring strJsonExtension = L".json";

	return strFileName.length() >= strJsonExtension.length() &&
		0 == _wcsicmp(strFileName.c_str() + strFileName.length() - strJsonExtension.length(), strJsonExtension.c_str());
}

static void EffectCatalog_CollectSourceFilesRecursive(const _wstring& strRootDirectory, const _wstring& strCurrentDirectory, vector<_wstring>* pOutRelativeFilePaths)
{
	if (nullptr == pOutRelativeFilePaths)
		return;

	const _wstring strSearchPath = strCurrentDirectory + L"\\*";
	WIN32_FIND_DATAW FindData = {};
	HANDLE hFind = FindFirstFileW(strSearchPath.c_str(), &FindData);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	do
	{
		if (0 == wcscmp(FindData.cFileName, L".") || 0 == wcscmp(FindData.cFileName, L".."))
			continue;

		const _wstring strFilePath = strCurrentDirectory + L"\\" + FindData.cFileName;

		if (0 != (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// 검색용 메타데이터 폴더는 런타임 Effect JSON 원본이 아니므로 목록에서 제외한다.
			if (0 == _wcsicmp(FindData.cFileName, L"Effect_Search"))
				continue;

			EffectCatalog_CollectSourceFilesRecursive(strRootDirectory, strFilePath, pOutRelativeFilePaths);
			continue;
		}

		const _wstring strFileName = FindData.cFileName;

		if (false == EffectCatalog_IsJsonFile(strFileName) || 0 == _wcsicmp(strFileName.c_str(), GameCore::CEffect_ResourceCatalog::Get_LibraryFileName().c_str()))
			continue;

		if (strFilePath.length() <= strRootDirectory.length() + 1)
			continue;

		pOutRelativeFilePaths->push_back(strFilePath.substr(strRootDirectory.length() + 1));
	}
	while (FindNextFileW(hFind, &FindData));

	FindClose(hFind);
}

const _wstring& GameCore::CEffect_ResourceCatalog::Get_LibraryFileName()
{
	static const _wstring strLibraryFileName = L"EffectLibrary.json";
	return strLibraryFileName;
}

HRESULT GameCore::CEffect_ResourceCatalog::Load(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pOutEntries, string* pOutErrorMessage)
{
	if (nullptr == pOutEntries || strEffectsDirectory.empty())
		return E_FAIL;

	pOutEntries->clear();

	if (nullptr != pOutErrorMessage)
		pOutErrorMessage->clear();

	const _wstring strLibraryPath = strEffectsDirectory + L"\\" + Get_LibraryFileName();
	ifstream File(strLibraryPath, ios::binary);

	if (false == File.is_open())
		return S_FALSE;

	try
	{
		json Root = {};
		File >> Root;

		if (false == Root.contains("Effects") || false == Root["Effects"].is_array())
		{
			if (nullptr != pOutErrorMessage)
				*pOutErrorMessage = "Effect library does not contain an Effects array.";

			return E_FAIL;
		}

		unordered_set<string> RegisteredKeys = {};

		for (const json& EffectValue : Root["Effects"])
		{
			if (false == EffectValue.is_object())
				continue;

			EFFECT_RESOURCE_CATALOG_ENTRY Entry = {};
			Entry.strKey = EffectValue.value("Key", string{});
			Entry.strRelativeFilePath = EffectCatalog_NormalizeRelativePath(Engine::StringToWString(EffectValue.value("File", string{})));

			if (Entry.strKey.empty() || false == Is_ValidRelativeFilePath(Entry.strRelativeFilePath))
				continue;

			if (false == RegisteredKeys.emplace(Entry.strKey).second)
			{
				if (nullptr != pOutErrorMessage)
					*pOutErrorMessage = "Effect library contains a duplicate key: " + Entry.strKey;

				return E_FAIL;
			}

			pOutEntries->push_back(Entry);
		}
	}
	catch (const json::exception& Exception)
	{
		if (nullptr != pOutErrorMessage)
			*pOutErrorMessage = Exception.what();

		pOutEntries->clear();
		return E_FAIL;
	}

	stable_sort(pOutEntries->begin(), pOutEntries->end(), [](const EFFECT_RESOURCE_CATALOG_ENTRY& Left, const EFFECT_RESOURCE_CATALOG_ENTRY& Right) { return Left.strKey < Right.strKey; });
	return S_OK;
}

HRESULT GameCore::CEffect_ResourceCatalog::Save(const _wstring& strEffectsDirectory, const vector<EFFECT_RESOURCE_CATALOG_ENTRY>& Entries, string* pOutErrorMessage)
{
	if (strEffectsDirectory.empty())
		return E_FAIL;

	if (nullptr != pOutErrorMessage)
		pOutErrorMessage->clear();

	json Root = {};
	Root["Version"] = 1;
	Root["Effects"] = json::array();
	unordered_set<string> RegisteredKeys = {};

	for (const EFFECT_RESOURCE_CATALOG_ENTRY& SourceEntry : Entries)
	{
		const _wstring strRelativeFilePath = EffectCatalog_NormalizeRelativePath(SourceEntry.strRelativeFilePath);

		if (SourceEntry.strKey.empty() || false == Is_ValidRelativeFilePath(strRelativeFilePath) || false == RegisteredKeys.emplace(SourceEntry.strKey).second)
		{
			if (nullptr != pOutErrorMessage)
				*pOutErrorMessage = "Effect library contains an invalid or duplicate key.";

			return E_FAIL;
		}

		Root["Effects"].push_back({ { "Key", SourceEntry.strKey }, { "File", Engine::WStringToString(strRelativeFilePath) } });
	}

	ofstream File(strEffectsDirectory + L"\\" + Get_LibraryFileName(), ios::binary | ios::trunc);

	if (false == File.is_open())
	{
		if (nullptr != pOutErrorMessage)
			*pOutErrorMessage = "Effect library file could not be opened for writing.";

		return E_FAIL;
	}

	const string strJson = Root.dump(2);
	const unsigned char Bom[] = { 0xEF, 0xBB, 0xBF };
	File.write(reinterpret_cast<const char*>(Bom), sizeof(Bom));

	for (const char Character : strJson)
	{
		if ('\n' == Character)
			File.put('\r');

		File.put(Character);
	}

	return File.good() ? S_OK : E_FAIL;
}

HRESULT GameCore::CEffect_ResourceCatalog::Rename_Key(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pEntries, const string& strOldKey, const string& strNewKey, string* pOutErrorMessage)
{
	if (nullptr == pEntries || strOldKey.empty() || strNewKey.empty())
		return E_FAIL;

	if (nullptr != pOutErrorMessage)
		pOutErrorMessage->clear();

	if (strOldKey == strNewKey)
		return S_FALSE;

	vector<EFFECT_RESOURCE_CATALOG_ENTRY> UpdatedEntries = *pEntries;
	auto OldEntryIter = find_if(UpdatedEntries.begin(), UpdatedEntries.end(), [&strOldKey](const EFFECT_RESOURCE_CATALOG_ENTRY& Entry) { return Entry.strKey == strOldKey; });

	if (UpdatedEntries.end() == OldEntryIter)
	{
		if (nullptr != pOutErrorMessage)
			*pOutErrorMessage = "Effect key was not found: " + strOldKey;

		return E_FAIL;
	}

	const auto DuplicateIter = find_if(UpdatedEntries.begin(), UpdatedEntries.end(), [&strNewKey](const EFFECT_RESOURCE_CATALOG_ENTRY& Entry) { return Entry.strKey == strNewKey; });

	if (UpdatedEntries.end() != DuplicateIter)
	{
		if (nullptr != pOutErrorMessage)
			*pOutErrorMessage = "Effect key already exists: " + strNewKey;

		return E_FAIL;
	}

	OldEntryIter->strKey = strNewKey;

	if (FAILED(Save(strEffectsDirectory, UpdatedEntries, pOutErrorMessage)))
		return E_FAIL;

	stable_sort(UpdatedEntries.begin(), UpdatedEntries.end(), [](const EFFECT_RESOURCE_CATALOG_ENTRY& Left, const EFFECT_RESOURCE_CATALOG_ENTRY& Right) { return Left.strKey < Right.strKey; });
	*pEntries = move(UpdatedEntries);
	return S_OK;
}

HRESULT GameCore::CEffect_ResourceCatalog::Delete_Key(const _wstring& strEffectsDirectory, vector<EFFECT_RESOURCE_CATALOG_ENTRY>* pEntries, const string& strKey, string* pOutErrorMessage)
{
	if (nullptr == pEntries || strKey.empty())
		return E_FAIL;

	if (nullptr != pOutErrorMessage)
		pOutErrorMessage->clear();

	vector<EFFECT_RESOURCE_CATALOG_ENTRY> UpdatedEntries = *pEntries;
	const auto EntryIter = find_if(UpdatedEntries.begin(), UpdatedEntries.end(), [&strKey](const EFFECT_RESOURCE_CATALOG_ENTRY& Entry) { return Entry.strKey == strKey; });

	if (UpdatedEntries.end() == EntryIter)
	{
		if (nullptr != pOutErrorMessage)
			*pOutErrorMessage = "Effect key was not found: " + strKey;

		return E_FAIL;
	}

	UpdatedEntries.erase(EntryIter);

	if (FAILED(Save(strEffectsDirectory, UpdatedEntries, pOutErrorMessage)))
		return E_FAIL;

	*pEntries = move(UpdatedEntries);
	return S_OK;
}

void GameCore::CEffect_ResourceCatalog::Collect_SourceFiles(const _wstring& strEffectsDirectory, vector<_wstring>* pOutRelativeFilePaths)
{
	if (nullptr == pOutRelativeFilePaths)
		return;

	pOutRelativeFilePaths->clear();

	if (strEffectsDirectory.empty())
		return;

	EffectCatalog_CollectSourceFilesRecursive(strEffectsDirectory, strEffectsDirectory, pOutRelativeFilePaths);
	stable_sort(pOutRelativeFilePaths->begin(), pOutRelativeFilePaths->end());
}

const GameCore::EFFECT_RESOURCE_CATALOG_ENTRY* GameCore::CEffect_ResourceCatalog::Find_ByKey(const vector<EFFECT_RESOURCE_CATALOG_ENTRY>& Entries, const string& strKey)
{
	const auto EntryIter = find_if(Entries.begin(), Entries.end(), [&strKey](const EFFECT_RESOURCE_CATALOG_ENTRY& Entry) { return Entry.strKey == strKey; });
	return Entries.end() == EntryIter ? nullptr : &(*EntryIter);
}

_wstring GameCore::CEffect_ResourceCatalog::Resolve_SourcePath(const _wstring& strEffectsDirectory, const _wstring& strRelativeFilePath)
{
	if (strEffectsDirectory.empty() || false == Is_ValidRelativeFilePath(strRelativeFilePath))
		return {};

	return strEffectsDirectory + L"\\" + EffectCatalog_NormalizeRelativePath(strRelativeFilePath);
}

_bool GameCore::CEffect_ResourceCatalog::Is_ValidRelativeFilePath(const _wstring& strRelativeFilePath)
{
	const _wstring strNormalizedPath = EffectCatalog_NormalizeRelativePath(strRelativeFilePath);

	return false == strNormalizedPath.empty() &&
		_wstring::npos == strNormalizedPath.find(L"..") &&
		_wstring::npos == strNormalizedPath.find(L':') &&
		L'\\' != strNormalizedPath.front() &&
		L'/' != strNormalizedPath.front() &&
		EffectCatalog_IsJsonFile(strNormalizedPath);
}

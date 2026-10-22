#include "UIAssetManager.h"
#include "UIAssetSerializer.h"
#include "Engine_Utility.h"

namespace
{
    using namespace Engine;

    HRESULT Set_Error(_wstring* pError, const _wstring& strMessage, HRESULT hr = E_FAIL)
    {
        if (pError != nullptr)
            *pError = strMessage;

        return hr;
    }
}

HRESULT CUIAssetManager::Load_Asset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation, _wstring* pError)
{
    if (strFilePath.empty())
        return Set_Error(pError, L"UI Asset path is empty.", E_FAIL);

    UI_ASSET_DESC Asset = {};

    if (FAILED(CUIAssetSerializer::Load(strFilePath, &Asset, pError)))
        return E_FAIL;

    return Register_Asset(move(Asset), pOutValidation, pError);
}

HRESULT CUIAssetManager::Register_Asset(UI_ASSET_DESC Asset, UI_ASSET_VALIDATION_RESULT* pOutValidation, _wstring* pError)
{
    UI_ASSET_VALIDATION_RESULT LocalValidation = {};

    UI_ASSET_VALIDATION_RESULT* pValidation = pOutValidation != nullptr ? pOutValidation : &LocalValidation;

    if (FAILED(CUIAssetValidator::Validate(Asset, pValidation)))
        return Set_Error(pError, L"UI Asset validation failed.");

    if (Asset.strAssetID.empty())
        return Set_Error(pError, L"UI Asset ID is empty.");

    if (m_Assets.find(Asset.strAssetID) != m_Assets.end())
        return Set_Error(pError, L"Duplicate UI Asset ID.");

    const string strAssetID = Asset.strAssetID;

    m_Assets.emplace(strAssetID, move(Asset));

    return S_OK;
}

// 폴더 안의 모든 *.ui.json을 검증한 뒤 한 번에 등록
HRESULT CUIAssetManager::Load_AssetsFromDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount, _wstring* pError)
{
    if(pOutLoadedCount != nullptr)
        *pOutLoadedCount = 0;

    if (strDirectoryPath.empty())
        return Set_Error(pError, L"UI Asset directory path is empty.");

    _wstring strNormalizedPath = strDirectoryPath;

    const wchar_t chLast = strNormalizedPath.back();

    if (chLast != L'\\' && chLast != L'/')
        strNormalizedPath += L'\\';

    const _wstring strSearchPattern = strNormalizedPath + L"*.ui.json";

    WIN32_FIND_DATAW FindData = {};

    HANDLE hFind = FindFirstFileW(strSearchPattern.c_str(), &FindData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return Set_Error(pError, L"No UI Asset files were found in directory: " + strNormalizedPath);
    }

    vector<_wstring> AssetFilePaths;

    do
    {
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            continue;
        }

        AssetFilePaths.push_back(strNormalizedPath + FindData.cFileName);

    } while (FindNextFileW(hFind, &FindData));

    FindClose(hFind);

    if (AssetFilePaths.empty())
    {
        return Set_Error(pError, L"No UI Asset files were found in directory: " + strNormalizedPath);
    }

    // 파일 탐색 순서가 실행 환경에 따라 달라지지 않도록 정렬
    sort(AssetFilePaths.begin(), AssetFilePaths.end());

    vector<UI_ASSET_DESC> PendingAssets;
    unordered_set<string> DirectoryAssetIDs;

    // 먼저 모든 파일을 읽고 검증, 하나라도 잘 못 되면 아직 에셋 매니저에 아무것도 등록하지 않음
    for (const _wstring& strFilePath : AssetFilePaths)
    {
        UI_ASSET_DESC Asset = {};
        _wstring strLoadError;

        if (FAILED(CUIAssetSerializer::Load(strFilePath, &Asset, &strLoadError)))
        {
            return Set_Error(pError, L"Failed to load UI Asset: " + strFilePath + L"\n" + strLoadError);
        }

        UI_ASSET_VALIDATION_RESULT Validation = {};

        if (FAILED(CUIAssetValidator::Validate(Asset, &Validation)))
        {
            return Set_Error(pError, L"UI Asset validation failed: " + strFilePath);
        }

        if (Asset.strAssetID.empty())
        {
            return Set_Error(pError, L"UI Asset ID is empty: " + strFilePath);
        }

        // 같은 폴더 안에 동일 AssetID 파일이 두 개 있으면 등록을 중단한다.
        if (!DirectoryAssetIDs.insert(Asset.strAssetID).second)
        {
            return Set_Error(pError, L"Duplicate UI Asset ID in directory: " + StringToWString(Asset.strAssetID));
        }

        // 이전 레벨에서 이미 등록한 Asset은 다시 등록하지 않는다.
        if (m_Assets.find(Asset.strAssetID) != m_Assets.end())
        {
            continue;
        }

        PendingAssets.push_back(move(Asset));
    }

    /*
     * 모든 파일의 로드와 검증이 성공한 뒤 실제 Registry에 반영한다.
     * 따라서 중간 파일 실패로 일부만 등록되는 상태를 막는다.
     */
    for (UI_ASSET_DESC& Asset : PendingAssets)
    {
        const string strAssetID = Asset.strAssetID;

        m_Assets.emplace(strAssetID, move(Asset));
    }

    if (pOutLoadedCount != nullptr)
        *pOutLoadedCount = PendingAssets.size();

    return S_OK;
}

const UI_ASSET_DESC* CUIAssetManager::Find_Asset(const string& strAssetID) const
{
    const auto Iter = m_Assets.find(strAssetID);

    if (Iter == m_Assets.end())
        return nullptr;

    return &Iter->second;
}

_bool CUIAssetManager::Contains_Asset(const string& strAssetID) const
{
    return m_Assets.find(strAssetID) != m_Assets.end(); 
}

void CUIAssetManager::Clear()
{
    m_Assets.clear();
}
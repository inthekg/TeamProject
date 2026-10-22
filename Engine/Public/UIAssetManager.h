#pragma once
#include "UIAssetValidator.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIAssetManager final
{
public:
	CUIAssetManager() = default;
	~CUIAssetManager() = default;

	CUIAssetManager(const CUIAssetManager&) = delete;
	CUIAssetManager& operator =(const CUIAssetManager&) = delete;

public:
	HRESULT					Load_Asset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation = nullptr, _wstring* pError = nullptr);
	HRESULT					Register_Asset(UI_ASSET_DESC Asset, UI_ASSET_VALIDATION_RESULT* pOutValidation = nullptr, _wstring* pError = 
		nullptr);
	HRESULT					Load_AssetsFromDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount = nullptr, _wstring* pError = nullptr);


	const UI_ASSET_DESC*	Find_Asset(const string& strAssetID) const;

	_bool					Contains_Asset(const string& strAssetID) const;

	size_t					Get_AssetCount() const { return m_Assets.size(); }

	void					Clear();

private:
	unordered_map<string, UI_ASSET_DESC> m_Assets;
};

NS_END
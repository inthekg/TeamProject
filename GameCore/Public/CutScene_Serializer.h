#pragma once

#include "Base.h"
#include "CutScene_Data.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CCutScene_Serializer final
{
public:
	static HRESULT Save(const _wstring& strFilePath, const CUTSCENE_ASSET& Asset, string* pErrorMessage = nullptr);
	static HRESULT Load(const _wstring& strFilePath, CUTSCENE_ASSET* pOutAsset, string* pErrorMessage = nullptr);
	static string Make_AssetSnapshot(const CUTSCENE_ASSET& Asset);
};

NS_END

#pragma once
#include "UI_Defines.h"

NS_BEGIN(Engine)

// Json 변환기
class ENGINE_DLL CUIAssetSerializer final
{
private:
	CUIAssetSerializer() = delete;
	~CUIAssetSerializer() = delete;

public:
	static HRESULT Save(const _wstring& strFilePath, const UI_ASSET_DESC& Asset, _wstring* pError = nullptr);
	static HRESULT Load(const _wstring& strFilePath, UI_ASSET_DESC* pOutAsset, _wstring* pError = nullptr);

	// 에디터와 테스트에서 파일 입출력 없이 사용할 변환 함수
	static json ToJson(const UI_ASSET_DESC& Asset);
	static HRESULT FromJson(const json& Root, UI_ASSET_DESC* pOutAsset, _wstring* pError = nullptr);

private:
	static json Write_Element(const UI_ELEMENT_ASSET_DESC& Element);
	static HRESULT Read_Element(const json& ElementJson, UI_ELEMENT_ASSET_DESC* pOutElement, _wstring* pError, const _wstring& strPath, _uint iDepth);
};

NS_END
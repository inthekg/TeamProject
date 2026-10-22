#pragma once
#include "UIAssetValidator.h"

NS_BEGIN(Engine)

struct UI_ASSET_CONVERSION_RESULT
{
	// v1 Object 하나가 v2 Asset 하나로 변환
	vector<UI_ASSET_DESC> Assets;

	// 자동 결정할 수 없거나 잘못된 데이터 목록
	UI_ASSET_VALIDATION_RESULT Diagnostics;
};

class ENGINE_DLL CUIAssetConverter final
{
private:
	CUIAssetConverter() = delete;
	~CUIAssetConverter() = delete;

public:
	static _bool	Is_V1Document(const json& LegacyRoot);
	static HRESULT	Convert_V1(const json& LegacyRoot, UI_ASSET_CONVERSION_RESULT* pOutResult, _wstring* pError = nullptr);
};

NS_END
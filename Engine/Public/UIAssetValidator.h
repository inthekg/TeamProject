#pragma once
#include "UI_AssetData.h"

NS_BEGIN(Engine)

enum class UI_ASSET_VALIDATION_SEVERITY
{
	UI_WARNING,
	UI_ERROR,
};

struct UI_ASSET_VALIDATION_ISSUE
{
	UI_ASSET_VALIDATION_SEVERITY	eSeverity = UI_ASSET_VALIDATION_SEVERITY::UI_ERROR;

	// 문제가 발생한 데이터 위치
	_wstring						strPath;

	// 사용자에게 표시할 내용
	_wstring						strMessage;
};

struct UI_ASSET_VALIDATION_RESULT
{
	vector<UI_ASSET_VALIDATION_ISSUE> Issues;

	_bool Has_Error() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.eSeverity == UI_ASSET_VALIDATION_SEVERITY::UI_ERROR)
				return true;
		}

		return false;
	}

	_bool Has_Warning() const
	{
		for (const auto& Issue : Issues)
		{
			if (Issue.eSeverity == UI_ASSET_VALIDATION_SEVERITY::UI_WARNING)
				return true;
		}

		return false;
	}
};


// UI 에셋 검증기
class ENGINE_DLL CUIAssetValidator final
{
private:
	CUIAssetValidator() = delete;
	~CUIAssetValidator() = delete;

public:
	static HRESULT Validate(const UI_ASSET_DESC& Asset, UI_ASSET_VALIDATION_RESULT* pOutResult);
};

NS_END
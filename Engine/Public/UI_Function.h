#pragma once
#include "UI_AssetData.h"

NS_BEGIN(Engine)

constexpr _uint UI_MAX_HIERARCHY_DEPTH = 64;

inline HRESULT Fail(_wstring* pError, const _wstring& strMessage)
{
	if (pError != nullptr)
		*pError = strMessage;

	return E_FAIL;
}

inline json WriteFloat2(const _float2& vValue)
{
	return json::array({ vValue.x, vValue.y });
}

inline json WriteFloat4(const _float4& vValue)
{
	return json::array({ vValue.x, vValue.y, vValue.z, vValue.w });
}

inline _bool ReadFloat2(const json& Value, _float2* pOutValue)
{
	if (pOutValue == nullptr || !Value.is_array() || Value.size() != 2 || !Value[0].is_number() || !Value[1].is_number())
		return false;

	pOutValue->x = Value[0].get<_float>();
	pOutValue->y = Value[1].get<_float>();

	return true;
}

inline _bool ReadFloat4(const json& Value, _float4* pOutValue)
{
	if (pOutValue == nullptr || !Value.is_array() || Value.size() != 4)
		return false;

	for (size_t i = 0; i < 4; i++)
	{
		if (!Value[i].is_number())
			return false;
	}

	pOutValue->x = Value[0].get<_float>();
	pOutValue->y = Value[1].get<_float>();
	pOutValue->z = Value[2].get<_float>();
	pOutValue->w = Value[3].get<_float>();

	return true;
}

inline _float2 Get_UIAnchorNormalized(UI_ANCHOR eAnchor)
{
	switch (eAnchor)
	{
	case UI_ANCHOR::LEFT_TOP:
		return _float2(0.f, 0.f);

	case UI_ANCHOR::RIGHT_TOP:
		return _float2(1.f, 0.f);

	case UI_ANCHOR::CENTER:
		return _float2(0.5f, 0.5f);

	case UI_ANCHOR::LEFT_BOTTOM:
		return _float2(0.f, 1.f);

	case UI_ANCHOR::RIGHT_BOTTOM:
		return _float2(1.f, 1.f);

	case UI_ANCHOR::TOP_CENTER:
		return _float2(0.5f, 0.f);

	case UI_ANCHOR::LEFT_CENTER:
		return _float2(0.f, 0.5f);

	case UI_ANCHOR::RIGHT_CENTER:
		return _float2(1.f, 0.5f);

	case UI_ANCHOR::BOTTOM_CENTER:
		return _float2(0.5f, 1.f);

	default:
		return _float2(0.5f, 0.5f);
	}
}

inline _float2 Calc_UIAnchorOffset(UI_ANCHOR eAnchor, const _float2& vParentSize, const _float2& vParentPivot)
{
	const _float2 vAnchor = Get_UIAnchorNormalized(eAnchor);

	return _float2((vAnchor.x - vParentPivot.x) * vParentSize.x, (vParentPivot.y - vAnchor.y) * vParentSize.y);
}

// 자신의 아래에 실제 자식 Element를 둘 수 있는 타입인지 검사
inline _bool Can_UIElementHaveChildren(UI_ELEMENT_TYPE eType)
{
	return eType == UI_ELEMENT_TYPE::CONTAINER || eType == UI_ELEMENT_TYPE::BUTTON;
}

#pragma region enum 문자열 변환
inline const char* ToString(UI_ASSET_KIND eValue)
{
	switch (eValue)
	{
	case UI_ASSET_KIND::SCREEN:
		return "Screen";

	case UI_ASSET_KIND::COMPONENT:
		return "Component";

	case UI_ASSET_KIND::PROJECTED:
		return "Projected";
	}

	throw invalid_argument("Invalid UI_ASSET_KIND.");
}

inline _bool FromString(const string& strValue, UI_ASSET_KIND* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Screen")
		*pOutValue = UI_ASSET_KIND::SCREEN;
	else if (strValue == "Component")
		*pOutValue = UI_ASSET_KIND::COMPONENT;
	else if (strValue == "Projected")
		*pOutValue = UI_ASSET_KIND::PROJECTED;
	else
		return false;

	return true;
}

inline const char* ToString(UI_INSTANCE_POLICY eValue)
{
	switch (eValue)
	{
	case UI_INSTANCE_POLICY::SINGLETON:
		return "Singleton";

	case UI_INSTANCE_POLICY::SINGLETON_LAZY:
		return "SingletonLazy";

	case UI_INSTANCE_POLICY::MULTIPLE:
		return "Multiple";

	case UI_INSTANCE_POLICY::POOLED_MULTIPLE:
		return "PooledMultiple";
	}

	throw invalid_argument("Invalid UI_INSTANCE_POLICY.");
}

inline _bool FromString(const string& strValue, UI_INSTANCE_POLICY* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Singleton")
		*pOutValue = UI_INSTANCE_POLICY::SINGLETON;
	else if (strValue == "SingletonLazy")
		*pOutValue = UI_INSTANCE_POLICY::SINGLETON_LAZY;
	else if (strValue == "Multiple")
		*pOutValue = UI_INSTANCE_POLICY::MULTIPLE;
	else if (strValue == "PooledMultiple")
		*pOutValue = UI_INSTANCE_POLICY::POOLED_MULTIPLE;
	else
		return false;

	return true;
}

inline const char* ToString(UI_LIFETIME eValue)
{
	switch (eValue)
	{
	case UI_LIFETIME::GLOBAL:
		return "Global";

	case UI_LIFETIME::GAMEPLAY:
		return "Gameplay";

	case UI_LIFETIME::CHAPTER:
		return "Chapter";

	case UI_LIFETIME::EXTERNAL:
		return "External";

	case UI_LIFETIME::TRANSIENT:
		return "Transient";
	}

	throw invalid_argument("Invalid UI_LIFETIME.");
}

inline _bool FromString(const string& strValue, UI_LIFETIME* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Global")
		*pOutValue = UI_LIFETIME::GLOBAL;
	else if (strValue == "Gameplay")
		*pOutValue = UI_LIFETIME::GAMEPLAY;
	else if (strValue == "Chapter")
		*pOutValue = UI_LIFETIME::CHAPTER;
	else if (strValue == "External")
		*pOutValue = UI_LIFETIME::EXTERNAL;
	else if (strValue == "Transient")
		*pOutValue = UI_LIFETIME::TRANSIENT;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ELEMENT_TYPE eValue)
{
	switch (eValue)
	{
	case UI_ELEMENT_TYPE::CONTAINER:
		return "Container";

	case UI_ELEMENT_TYPE::IMAGE:
		return "Image";

	case UI_ELEMENT_TYPE::PROGRESS_BAR:
		return "ProgressBar";

	case UI_ELEMENT_TYPE::TEXT:
		return "Text";

	case UI_ELEMENT_TYPE::BUTTON:
		return "Button";

	case UI_ELEMENT_TYPE::COMPONENT_INSTANCE:
		return "ComponentInstance";
	}

	throw invalid_argument("Invalid UI_ELEMENT_TYPE.");
}

inline _bool FromString(const string& strValue, UI_ELEMENT_TYPE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Container")
		*pOutValue = UI_ELEMENT_TYPE::CONTAINER;
	else if (strValue == "Image")
		*pOutValue = UI_ELEMENT_TYPE::IMAGE;
	else if (strValue == "ProgressBar")
		*pOutValue = UI_ELEMENT_TYPE::PROGRESS_BAR;
	else if (strValue == "Text")
		*pOutValue = UI_ELEMENT_TYPE::TEXT;
	else if (strValue == "Button")
		*pOutValue = UI_ELEMENT_TYPE::BUTTON;
	else if (strValue == "ComponentInstance")
		*pOutValue = UI_ELEMENT_TYPE::COMPONENT_INSTANCE;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ASSET_IMAGE_DRAW_TYPE eValue)
{
	switch (eValue)
	{
	case UI_ASSET_IMAGE_DRAW_TYPE::NORMAL:
		return "Normal";

	case UI_ASSET_IMAGE_DRAW_TYPE::NINE_SLICE:
		return "NineSlice";
	}

	throw invalid_argument("Invalid UI_ASSET_IMAGE_DRAW_TYPE.");
}

inline _bool FromString(const string& strValue, UI_ASSET_IMAGE_DRAW_TYPE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Normal")
		*pOutValue = UI_ASSET_IMAGE_DRAW_TYPE::NORMAL;
	else if (strValue == "NineSlice")
		*pOutValue = UI_ASSET_IMAGE_DRAW_TYPE::NINE_SLICE;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ASSET_PROGRESS_SLOT_TYPE eValue)
{
	switch (eValue)
	{
	case UI_ASSET_PROGRESS_SLOT_TYPE::BACKGROUND:
		return "Background";

	case UI_ASSET_PROGRESS_SLOT_TYPE::FILL:
		return "Fill";

	case UI_ASSET_PROGRESS_SLOT_TYPE::FX:
		return "FX";
	}

	throw invalid_argument("Invalid UI_ASSET_PROGRESS_SLOT_TYPE.");
}

inline _bool FromString(const string& strValue, UI_ASSET_PROGRESS_SLOT_TYPE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Background")
		*pOutValue = UI_ASSET_PROGRESS_SLOT_TYPE::BACKGROUND;
	else if (strValue == "Fill")
		*pOutValue = UI_ASSET_PROGRESS_SLOT_TYPE::FILL;
	else if (strValue == "FX")
		*pOutValue = UI_ASSET_PROGRESS_SLOT_TYPE::FX;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ASSET_PROGRESS_BLEND_MODE eValue)
{
	switch (eValue)
	{
	case UI_ASSET_PROGRESS_BLEND_MODE::ALPHA:
		return "Alpha";

	case UI_ASSET_PROGRESS_BLEND_MODE::ADDITIVE:
		return "Additive";
	}

	throw invalid_argument("Invalid UI_ASSET_PROGRESS_BLEND_MODE.");
}

inline _bool FromString(const string& strValue, UI_ASSET_PROGRESS_BLEND_MODE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Alpha")
		*pOutValue = UI_ASSET_PROGRESS_BLEND_MODE::ALPHA;
	else if (strValue == "Additive")
		*pOutValue = UI_ASSET_PROGRESS_BLEND_MODE::ADDITIVE;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ASSET_TEXT_HORIZONTAL_ALIGN eValue)
{
	switch (eValue)
	{
	case UI_ASSET_TEXT_HORIZONTAL_ALIGN::LEFT:
		return "Left";

	case UI_ASSET_TEXT_HORIZONTAL_ALIGN::CENTER:
		return "Center";

	case UI_ASSET_TEXT_HORIZONTAL_ALIGN::RIGHT:
		return "Right";
	}

	throw invalid_argument("Invalid UI_ASSET_TEXT_HORIZONTAL_ALIGN.");
}

inline _bool FromString(const string& strValue, UI_ASSET_TEXT_HORIZONTAL_ALIGN* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Left")
		*pOutValue = UI_ASSET_TEXT_HORIZONTAL_ALIGN::LEFT;
	else if (strValue == "Center")
		*pOutValue = UI_ASSET_TEXT_HORIZONTAL_ALIGN::CENTER;
	else if (strValue == "Right")
		*pOutValue = UI_ASSET_TEXT_HORIZONTAL_ALIGN::RIGHT;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ASSET_TEXT_VERTICAL_ALIGN eValue)
{
	switch (eValue)
	{
	case UI_ASSET_TEXT_VERTICAL_ALIGN::TOP:
		return "Top";

	case UI_ASSET_TEXT_VERTICAL_ALIGN::CENTER:
		return "Center";

	case UI_ASSET_TEXT_VERTICAL_ALIGN::BOTTOM:
		return "Bottom";
	}

	throw invalid_argument("Invalid UI_ASSET_TEXT_VERTICAL_ALIGN.");
}

inline _bool FromString(const string& strValue, UI_ASSET_TEXT_VERTICAL_ALIGN* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Top")
		*pOutValue = UI_ASSET_TEXT_VERTICAL_ALIGN::TOP;
	else if (strValue == "Center")
		*pOutValue = UI_ASSET_TEXT_VERTICAL_ALIGN::CENTER;
	else if (strValue == "Bottom")
		*pOutValue = UI_ASSET_TEXT_VERTICAL_ALIGN::BOTTOM;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ANCHOR eValue)
{
	switch (eValue)
	{
	case UI_ANCHOR::LEFT_TOP:
		return "LeftTop";

	case UI_ANCHOR::RIGHT_TOP:
		return "RightTop";

	case UI_ANCHOR::CENTER:
		return "Center";

	case UI_ANCHOR::LEFT_BOTTOM:
		return "LeftBottom";

	case UI_ANCHOR::RIGHT_BOTTOM:
		return "RightBottom";

	case UI_ANCHOR::TOP_CENTER:
		return "TopCenter";

	case UI_ANCHOR::LEFT_CENTER:
		return "LeftCenter";

	case UI_ANCHOR::RIGHT_CENTER:
		return "RightCenter";

	case UI_ANCHOR::BOTTOM_CENTER:
		return "BottomCenter";
	}

	throw invalid_argument("Invalid UI_ANCHOR.");
}

inline _bool FromString(const string& strValue, UI_ANCHOR* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "LeftTop")
		*pOutValue = UI_ANCHOR::LEFT_TOP;
	else if (strValue == "RightTop")
		*pOutValue = UI_ANCHOR::RIGHT_TOP;
	else if (strValue == "Center")
		*pOutValue = UI_ANCHOR::CENTER;
	else if (strValue == "LeftBottom")
		*pOutValue = UI_ANCHOR::LEFT_BOTTOM;
	else if (strValue == "RightBottom")
		*pOutValue = UI_ANCHOR::RIGHT_BOTTOM;
	else if (strValue == "TopCenter")
		*pOutValue = UI_ANCHOR::TOP_CENTER;
	else if (strValue == "LeftCenter")
		*pOutValue = UI_ANCHOR::LEFT_CENTER;
	else if (strValue == "RightCenter")
		*pOutValue = UI_ANCHOR::RIGHT_CENTER;
	else if (strValue == "BottomCenter")
		*pOutValue = UI_ANCHOR::BOTTOM_CENTER;
	else
		return false;

	return true;
}

inline const char* ToString(UI_PROGRESS_FILL_TYPE eValue)
{
	switch (eValue)
	{
	case UI_PROGRESS_FILL_TYPE::HORIZONTAL:
		return "Horizontal";

	case UI_PROGRESS_FILL_TYPE::VERTICAL:
		return "Vertical";

	case UI_PROGRESS_FILL_TYPE::RADIAL:
		return "Radial";
	}

	throw invalid_argument("Invalid UI_PROGRESS_FILL_TYPE.");
}

inline _bool FromString(const string& strValue, UI_PROGRESS_FILL_TYPE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Horizontal")
		*pOutValue = UI_PROGRESS_FILL_TYPE::HORIZONTAL;
	else if (strValue == "Vertical")
		*pOutValue = UI_PROGRESS_FILL_TYPE::VERTICAL;
	else if (strValue == "Radial")
		*pOutValue = UI_PROGRESS_FILL_TYPE::RADIAL;
	else
		return false;

	return true;
}

inline const char* ToString(UI_PROGRESS_FILL_DIRECTION eValue)
{
	switch (eValue)
	{
	case UI_PROGRESS_FILL_DIRECTION::LEFT_TO_RIGHT:
		return "LeftToRight";

	case UI_PROGRESS_FILL_DIRECTION::RIGHT_TO_LEFT:
		return "RightToLeft";

	case UI_PROGRESS_FILL_DIRECTION::TOP_TO_BOTTOM:
		return "TopToBottom";

	case UI_PROGRESS_FILL_DIRECTION::BOTTOM_TO_TOP:
		return "BottomToTop";

	case UI_PROGRESS_FILL_DIRECTION::CLOCKWISE:
		return "Clockwise";

	case UI_PROGRESS_FILL_DIRECTION::COUNTER_CLOCKWISE:
		return "CounterClockwise";
	}

	throw invalid_argument("Invalid UI_PROGRESS_FILL_DIRECTION.");
}

inline _bool FromString(const string& strValue, UI_PROGRESS_FILL_DIRECTION* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "LeftToRight")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::LEFT_TO_RIGHT;
	else if (strValue == "RightToLeft")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::RIGHT_TO_LEFT;
	else if (strValue == "TopToBottom")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::TOP_TO_BOTTOM;
	else if (strValue == "BottomToTop")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::BOTTOM_TO_TOP;
	else if (strValue == "Clockwise")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::CLOCKWISE;
	else if (strValue == "CounterClockwise")
		*pOutValue = UI_PROGRESS_FILL_DIRECTION::COUNTER_CLOCKWISE;
	else
		return false;

	return true;
}

inline const char* ToString(UI_PROGRESS_RESIZE_DIRECTION eValue)
{
	switch (eValue)
	{
	case UI_PROGRESS_RESIZE_DIRECTION::RIGHT:
		return "Right";

	case UI_PROGRESS_RESIZE_DIRECTION::LEFT:
		return "Left";

	case UI_PROGRESS_RESIZE_DIRECTION::CENTER:
		return "Center";

	case UI_PROGRESS_RESIZE_DIRECTION::UP:
		return "Up";

	case UI_PROGRESS_RESIZE_DIRECTION::DOWN:
		return "Down";
	}

	throw invalid_argument("Invalid UI_PROGRESS_RESIZE_DIRECTION.");
}

inline _bool FromString(const string& strValue, UI_PROGRESS_RESIZE_DIRECTION* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Right")
		*pOutValue = UI_PROGRESS_RESIZE_DIRECTION::RIGHT;
	else if (strValue == "Left")
		*pOutValue = UI_PROGRESS_RESIZE_DIRECTION::LEFT;
	else if (strValue == "Center")
		*pOutValue = UI_PROGRESS_RESIZE_DIRECTION::CENTER;
	else if (strValue == "Up")
		*pOutValue = UI_PROGRESS_RESIZE_DIRECTION::UP;
	else if (strValue == "Down")
		*pOutValue = UI_PROGRESS_RESIZE_DIRECTION::DOWN;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ANIMATION_PROPERTY eValue)
{
	switch (eValue)
	{
	case UI_ANIMATION_PROPERTY::POSITION:
		return "Position";

	case UI_ANIMATION_PROPERTY::SIZE:
		return "Size";

	case UI_ANIMATION_PROPERTY::SCALE:
		return "Scale";

	case UI_ANIMATION_PROPERTY::ROTATION:
		return "Rotation";

	case UI_ANIMATION_PROPERTY::COLOR:
		return "Color";

	case UI_ANIMATION_PROPERTY::ALPHA:
		return "Alpha";

	case UI_ANIMATION_PROPERTY::UV_RECT:
		return "UVRect";

	case UI_ANIMATION_PROPERTY::REVEAL_PROGRESS:
		return "RevealProgress";
	}

	throw invalid_argument("Invalid UI_ANIMATION_PROPERTY.");
}

inline _bool FromString(const string& strValue, UI_ANIMATION_PROPERTY* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Position")
		*pOutValue = UI_ANIMATION_PROPERTY::POSITION;
	else if (strValue == "Size")
		*pOutValue = UI_ANIMATION_PROPERTY::SIZE;
	else if (strValue == "Scale")
		*pOutValue = UI_ANIMATION_PROPERTY::SCALE;
	else if (strValue == "Rotation")
		*pOutValue = UI_ANIMATION_PROPERTY::ROTATION;
	else if (strValue == "Color")
		*pOutValue = UI_ANIMATION_PROPERTY::COLOR;
	else if (strValue == "Alpha")
		*pOutValue = UI_ANIMATION_PROPERTY::ALPHA;
	else if (strValue == "UVRect")
		*pOutValue = UI_ANIMATION_PROPERTY::UV_RECT;
	else if (strValue == "RevealProgress")
		*pOutValue = UI_ANIMATION_PROPERTY::REVEAL_PROGRESS;
	else
		return false;

	return true;
}

inline const char* ToString(UI_ANIMATION_INTERPOLATION eValue)
{
	switch (eValue)
	{
	case UI_ANIMATION_INTERPOLATION::STEP:
		return "Step";

	case UI_ANIMATION_INTERPOLATION::LINEAR:
		return "Linear";

	case UI_ANIMATION_INTERPOLATION::EASE_IN:
		return "EaseIn";

	case UI_ANIMATION_INTERPOLATION::EASE_OUT:
		return "EaseOut";

	case UI_ANIMATION_INTERPOLATION::EASE_IN_OUT:
		return "EaseInOut";
	}

	throw invalid_argument("Invalid UI_ANIMATION_INTERPOLATION.");
}

inline _bool FromString(const string& strValue, UI_ANIMATION_INTERPOLATION* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "Step")
		*pOutValue = UI_ANIMATION_INTERPOLATION::STEP;
	else if (strValue == "Linear")
		*pOutValue = UI_ANIMATION_INTERPOLATION::LINEAR;
	else if (strValue == "EaseIn")
		*pOutValue = UI_ANIMATION_INTERPOLATION::EASE_IN;
	else if (strValue == "EaseOut")
		*pOutValue = UI_ANIMATION_INTERPOLATION::EASE_OUT;
	else if (strValue == "EaseInOut")
		*pOutValue = UI_ANIMATION_INTERPOLATION::EASE_IN_OUT;
	else
		return false;

	return true;
}

inline const char* ToString(UI_REVEAL_TYPE eValue)
{
	switch (eValue)
	{
	case UI_REVEAL_TYPE::NONE:
		return "None";

	case UI_REVEAL_TYPE::RADIAL:
		return "Radial";

	case UI_REVEAL_TYPE::DIAGONAL:
		return "Diagonal";

	case UI_REVEAL_TYPE::DISSOLVE:
		return "Dissolve";
	}

	return "None";
}

inline _bool FromString(const string& strValue, UI_REVEAL_TYPE* pOutValue)
{
	if (pOutValue == nullptr)
		return false;

	if (strValue == "None")
		*pOutValue = UI_REVEAL_TYPE::NONE;
	else if (strValue == "Radial")
		*pOutValue = UI_REVEAL_TYPE::RADIAL;
	else if (strValue == "Diagonal")
		*pOutValue = UI_REVEAL_TYPE::DIAGONAL;
	else if (strValue == "Dissolve")
		*pOutValue = UI_REVEAL_TYPE::DISSOLVE;
	else
		return false;

	return true;
}
#pragma endregion

NS_END
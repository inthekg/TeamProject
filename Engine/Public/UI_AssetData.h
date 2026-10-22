#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

// 하나의 컴포넌트 그룹 안에서 사용할 엘리먼트/슬롯 정렬 범위
constexpr _uint UI_ELEMENT_LOCAL_SORT_RANGE = 1000;

// 하나의 UI 에셋이 사용할 전체 정렬 범위, ComponentGroup 0~999 × LocalSort 0~999
constexpr _uint UI_ELEMENT_SORT_RANGE = UI_ELEMENT_LOCAL_SORT_RANGE * UI_ELEMENT_LOCAL_SORT_RANGE;

enum class UI_ANCHOR
{
	LEFT_TOP,
	RIGHT_TOP,
	CENTER,
	LEFT_BOTTOM,
	RIGHT_BOTTOM,

	TOP_CENTER,
	LEFT_CENTER,
	RIGHT_CENTER,
	BOTTOM_CENTER,
};

enum class UI_PROGRESS_FILL_TYPE
{
	HORIZONTAL,
	VERTICAL,
	RADIAL,
};

enum class UI_PROGRESS_FILL_DIRECTION
{
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT,
	TOP_TO_BOTTOM,
	BOTTOM_TO_TOP,
	CLOCKWISE,
	COUNTER_CLOCKWISE,
};

enum class UI_PROGRESS_RESIZE_DIRECTION
{
	RIGHT,
	LEFT,
	CENTER,
	UP,
	DOWN,
};

struct UITransform
{
	_float2		vPosition = {};
	_float2		vSize = { 100.f, 100.f };
	_float2		vPivot = { 0.5f, 0.5f };

	UI_ANCHOR	eAnchor = UI_ANCHOR::LEFT_TOP;

	_float		fRotation = {};
	_float2		vScale = { 1.f, 1.f };
};

// UI 이미지가 표시되는 영역을 제어하는 방식
enum class UI_REVEAL_TYPE
{
	NONE,					// Reveal을 사용하지 않고 전체 이미지를 표시
	RADIAL,					// 중심에서 바깥으로 원형 확산
	DIAGONAL,				// 우측 상단에서 좌측 하단 방향으로 확산
	DISSOLVE,				// 노이즈 텍스처를 사용한 디졸브
};

struct UIRenderData
{
	_float4				vColor = { 1.f, 1.f, 1.f, 1.f };
	_float4				vUVRect = { 0.f, 0.f, 1.f, 1.f };

	_uint				iSortOrder = {};
	_bool				bVisible = { true };
	_bool				bUseTexture = { true };

	UI_REVEAL_TYPE		eRevealType = UI_REVEAL_TYPE::NONE;
	_float				fRevealProgress = { 1.f };
	_float				fRevealSoftness = { 0.02f };
	_float2				vRevealCenter = { 0.5f, 0.5f };

	class CShader*		pShader = { nullptr };
	class CVIBuffer*	pVIBuffer = { nullptr };
	class CMaterial*	pMaterial = { nullptr };
};

// 버전 2
constexpr _uint UI_SCHEMA_VERSION = 2;

enum class UI_ASSET_KIND
{
	SCREEN,
	COMPONENT,
	PROJECTED,
}; 

enum class UI_INSTANCE_POLICY
{
	SINGLETON,
	SINGLETON_LAZY,
	MULTIPLE,
	POOLED_MULTIPLE,
};

enum class UI_LIFETIME
{
	GLOBAL,
	GAMEPLAY,
	CHAPTER,
	EXTERNAL,
	TRANSIENT,
};

enum class UI_ELEMENT_TYPE
{
	CONTAINER,
	IMAGE,
	PROGRESS_BAR,
	TEXT,
	BUTTON,

	// 다른 Reusable Component Asset을 참조하는 엘리먼트
	COMPONENT_INSTANCE,
};

enum class UI_ASSET_IMAGE_DRAW_TYPE
{
	NORMAL,
	NINE_SLICE,
};

enum class UI_ASSET_PROGRESS_SLOT_TYPE
{
	BACKGROUND,
	FILL,
	FX,
};

enum class UI_ASSET_PROGRESS_BLEND_MODE
{
	ALPHA,
	ADDITIVE,
};

// Text의 가로 정렬 방식
enum class UI_ASSET_TEXT_HORIZONTAL_ALIGN
{
	LEFT,
	CENTER,
	RIGHT,
};

// Text의 세로 정렬 방식
enum class UI_ASSET_TEXT_VERTICAL_ALIGN
{
	TOP,
	CENTER,
	BOTTOM,
};

// UI 애니메이션으로 조절할 수 있는 Element 속성
enum class UI_ANIMATION_PROPERTY
{
	POSITION,
	SIZE,
	SCALE,
	ROTATION,
	COLOR,
	ALPHA,
	UV_RECT,
	REVEAL_PROGRESS,
};

// 현재 Keyframe부터 다음 Keyframe까지 사용할 보간 방식
enum class UI_ANIMATION_INTERPOLATION
{
	STEP,
	LINEAR,
	EASE_IN,
	EASE_OUT,
	EASE_IN_OUT,
};


// 애니메이션 Keyframe 하나
struct UI_ANIMATION_KEYFRAME_ASSET_DESC
{
	_float							fTime = {};

	/*
	 * 속성에 따라 사용하는 성분이 다르다.
	 * Position, Size, Scale : x, y
	 * Rotation, Alpha, RevealProgress : x
	 * Color, UVRect : x, y, z, w
	 */
	_float4							vValue = {};

	// 이 Keyframe에서 다음 Keyframe으로 진행할 때 사용할 보간 방식
	UI_ANIMATION_INTERPOLATION		eInterpolation = UI_ANIMATION_INTERPOLATION::LINEAR;
};

// 특정 Element의 특정 속성을 제어하는 Track
struct UI_ANIMATION_TRACK_ASSET_DESC
{
	/*
	 * BindingKey는 이름 변경이나 Component Namespace에 영향을 받을 수 있으므로
	 * 에셋 내부에서 고유한 ElementID를 애니메이션 대상으로 사용한다.
	 */
	_uint										iTargetElementID = { UINT_MAX };

	UI_ANIMATION_PROPERTY						eProperty = UI_ANIMATION_PROPERTY::POSITION;

	vector<UI_ANIMATION_KEYFRAME_ASSET_DESC>	Keyframes;
};

// 재생 가능한 애니메이션 Clip 하나
struct UI_ANIMATION_CLIP_ASSET_DESC
{
	// Controller에서 Play_Animation("Open")처럼 호출할 이름
	string									strName;

	_float									fDuration = { 1.f };
	_bool									bLoop = { false };

	vector<UI_ANIMATION_TRACK_ASSET_DESC>	Tracks;
};

// 순수 저장 데이터
struct UI_ASSET_RENDER_DESC
{
	_float4				vColor = { 1.f, 1.f, 1.f, 1.f };
	_float4				vUVRect = { 0.f, 0.f, 1.f, 1.f };

	_uint				iSortOrder = {};
	_bool				bVisible = { true };
	_bool				bUseTexture = { true };

	UI_REVEAL_TYPE		eRevealType = UI_REVEAL_TYPE::NONE;		// Reveal 모양
	_float				fRevealProgress = { 1.f };				// 0은 숨김, 1은 전체 표시
	_float				fRevealSoftness = { 0.02f };			// Reveal 경계의 부드러운 정도
	_float2				vRevealCenter = { 0.5f, 0.5f };			// 원형 Reveal 중심 UV
};

struct UI_IMAGE_ASSET_DESC
{
	string						strTexturePath;
	string						strDissolveTexturePath;		// 디졸브 마스크로 사용할 흑백 노이즈 텍스처

	UI_ASSET_IMAGE_DRAW_TYPE	eDrawType = UI_ASSET_IMAGE_DRAW_TYPE::NORMAL;

	_float4						vSliceMargin = { 0.f, 0.f, 0.f, 0.f };
	_float2						vTextureSize = { 1.f, 1.f };
	_float2						vSliceBaseSize = { 1.f, 1.f };
};

struct UI_PROGRESS_SLOT_ASSET_DESC
{
	_int							iSlotID = { -1 };
	_int							iTargetSlotID = { -1 };

	string							strName;
	string							strBindingKey;
	string							strTexturePath;

	UI_ASSET_PROGRESS_SLOT_TYPE		eType = UI_ASSET_PROGRESS_SLOT_TYPE::FILL;

	UI_ASSET_PROGRESS_BLEND_MODE	eBlendMode = UI_ASSET_PROGRESS_BLEND_MODE::ALPHA;

	_float							fAmount = { 1.f };
	_float							fStartAmount = {};
	_float							fEndAmount = { 1.f };

	_float4							vUVRect = { 0.f, 0.f, 1.f, 1.f };

	_bool							bStretchUVToRange = { false };

	_float4							vColor = { 1.f, 1.f, 1.f, 1.f };
	_float4							vColorLT = { 1.f, 1.f, 1.f, 1.f };
	_float4							vColorRT = { 1.f, 1.f, 1.f, 1.f };
	_float4							vColorRB = { 1.f, 1.f, 1.f, 1.f };
	_float4							vColorLB = { 1.f, 1.f, 1.f, 1.f };

	_int							iSortOffset = {};
	_bool							bVisible = { true };
};

struct UI_PROGRESS_ASSET_DESC
{
	_float							fLengthScale = { 1.f };

	UI_PROGRESS_FILL_TYPE			eFillType = UI_PROGRESS_FILL_TYPE::HORIZONTAL;

	UI_PROGRESS_FILL_DIRECTION		eFillDirection = UI_PROGRESS_FILL_DIRECTION::LEFT_TO_RIGHT;

	UI_PROGRESS_RESIZE_DIRECTION	eResizeDirection = UI_PROGRESS_RESIZE_DIRECTION::RIGHT;

	vector<UI_PROGRESS_SLOT_ASSET_DESC> Slots;
};

// Text Element가 JSON에 저장할 순수 데이터
struct UI_TEXT_ASSET_DESC
{
	// 실제 파일 경로가 아니라 Font Registry에서 찾을 고유 ID
	string									strFontAssetID = "DefaultUI";

	// 에디터 미리보기와 런타임 초기 표시 문자열
	// UTF-8 문자열로 저장
	string									strPreviewText = "Text";

	// 기준 해상도에서 의도하는 Text 높이
	// SpriteFont의 기본 LineSpacing과 비교해 렌더 Scale을 계산
	_float									fFontSize = 32.f;

	// 기본 줄 간격에 곱할 배율
	_float                                  fLineSpacing = 1.f;

	UI_ASSET_TEXT_HORIZONTAL_ALIGN          eHorizontalAlign = UI_ASSET_TEXT_HORIZONTAL_ALIGN::LEFT;
	UI_ASSET_TEXT_VERTICAL_ALIGN            eVerticalAlign = UI_ASSET_TEXT_VERTICAL_ALIGN::TOP;

	// Transform.vSize 영역을 기준으로 자동 줄바꿈할지 여부
	_bool                                   bWrap = false;
};

// Button 상태에서 자식 Element 하나에 적용할 외형 변경값
struct UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC
{
	// Button 하위 Element의 BindingKey
	string	strTargetBindingKey;

	// 해당 속성을 상태값으로 덮어쓸지 여부
	_bool	bOverrideVisible = { false };
	_bool	bVisible = { true };

	_bool	bOverrideColor = { false };
	_float4	vColor = { 1.f, 1.f, 1.f, 1.f };

	_bool	bOverrideScale = { false };
	_float2	vScale = { 1.f, 1.f };
};

// 하나의 Button 상태가 적용할 전체 자식 Override 목록
struct UI_BUTTON_VISUAL_STATE_ASSET_DESC
{
	vector<UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC> Overrides;
};

// Button Element가 JSON에 저장할 순수 데이터
struct UI_BUTTON_ASSET_DESC
{
	// Button이 확정됐을 때 Controller로 전달할 명령
	string	strCommandKey;

	// false면 화면에는 표시되지만 입력과 Command를 받지 않음
	_bool	bInteractable = { true };

	// 마우스가 올라오는 즉시 Focus 대상으로 지정할지 여부
	_bool	bFocusOnHover = { false };

	// Focus를 받았을 때 Selected 상태도 함께 적용할지 여부
	_bool   bSelectOnFocus = { false };

	// Mouse Up이 아닌 Mouse Down 순간 Command를 실행할지 여부
	_bool   bCommandOnPress = { false };
	
	// 키보드 Focus 탐색 그룹
	string	strFocusGroup = "Default";

	// 같은 FocusGroup 안에서의 탐색 순서
	_int	iFocusOrder = {};

	/*
	 * Normal 상태는 별도로 저장하지 않는다.
	 * 자식 Element의 기본 Visible, Color, Scale 값이 Normal이다.
	 */
	UI_BUTTON_VISUAL_STATE_ASSET_DESC Hovered;
	UI_BUTTON_VISUAL_STATE_ASSET_DESC Pressed;
	UI_BUTTON_VISUAL_STATE_ASSET_DESC Selected;		// 인벤토리 탭처럼 현재 선택이 유지되는 상태
	UI_BUTTON_VISUAL_STATE_ASSET_DESC Focused;		// 키보드, 게임패드가 현재 가리키는 상태
	UI_BUTTON_VISUAL_STATE_ASSET_DESC Disabled;
};

// 다른 Reusable Component Asset을 참조하기 위한 저장 데이터
struct UI_COMPONENT_INSTANCE_ASSET_DESC
{
	// 파일 경로가 아니라 UI ASSET의 고유 ID를 저장
	string strComponentAssetID;
};

struct UI_ELEMENT_ASSET_DESC
{
	_uint								iElementID = {};

	// 에디터 표시용 이름
	string								strName;

	// 컨트롤러와 연결되는 런타임 계약
	string								strBindingKey;

	UI_ELEMENT_TYPE						eType = UI_ELEMENT_TYPE::CONTAINER;

	UITransform							Transform;
	UI_ASSET_RENDER_DESC				Render;

	// eType에 해당하는 데이터만 사용
	UI_IMAGE_ASSET_DESC					Image;
	UI_PROGRESS_ASSET_DESC				Progress;
	UI_TEXT_ASSET_DESC					Text;
	UI_BUTTON_ASSET_DESC				Button;
	UI_COMPONENT_INSTANCE_ASSET_DESC	ComponentInstance;

	// 실제 Element Hierarchy
	vector<UI_ELEMENT_ASSET_DESC>		Children;
};

struct UI_PROJECTED_ASSET_DESC
{
	string	strAnchorSource;
	_float2	vScreenOffset = { 0.f, 0.f };

	_bool	bHideOffscreen = { true };
	_bool	bUseDistanceScale = { false };
};

struct UI_ASSET_DESC
{
	_uint						iSchemaVersion = UI_SCHEMA_VERSION;

	string						strAssetID;
	string						strName;

	UI_ASSET_KIND				eKind = UI_ASSET_KIND::SCREEN;

	// UI가 속할 렌더/표시 계층
	string						strLayer = "HUD";

	// 같은 Layer 안에서 Asset 단위 정렬
	_uint						iSortOrder = {};


	UI_INSTANCE_POLICY			eInstancePolicy = UI_INSTANCE_POLICY::SINGLETON;

	UI_LIFETIME					eLifetime = UI_LIFETIME::GAMEPLAY;

	// 엔진은 문자열만 보관 실제 생성 함수는 게임코어 팩토리에서 등록
	string						strControllerType;

	_float2						vReferenceResolution = { 1280.f, 720.f };

	_float						fMatch = 0.5f;

	UI_PROJECTED_ASSET_DESC		Projected;
	UI_ELEMENT_ASSET_DESC		Root;

	// 이 UI 에셋이 보유한 애니메이션 Clip 목록
	vector<UI_ANIMATION_CLIP_ASSET_DESC>		Animations;
};


NS_END
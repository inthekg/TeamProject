#pragma once
#include "UI_AssetData.h"

NS_BEGIN(Engine)

class CUIObject;
class CUIControllerBase;

// 런타임 UI 식별자
struct UI_HANDLE
{
	// 런타임 슬롯 번호
	// UINT_MAX는 어떤 슬롯도 가리키지 않는 무효 인덱스
	_uint iIndex = UINT_MAX;

	// 슬롯이 몇 번째로 사용 중인지
	// 0은 무효 세대로 예약
	_uint iGeneration = {};

	// 핸들 값 자체가 무효 기본값이 아닌지 검사
	_bool Is_Valid() const 
	{ 
		return iIndex != UINT_MAX && iGeneration != 0; 
	}

	void Invalidate()
	{
		iIndex = UINT_MAX;
		iGeneration = 0;
	}

	_bool operator ==(const UI_HANDLE& Other) const
	{
		return iIndex == Other.iIndex && iGeneration == Other.iGeneration;
	}

	_bool operator !=(const UI_HANDLE& Other) const
	{
		return !(*this == Other);
	}
};

enum class UI_RUNTIME_STATE
{	
	PENDING_SPAWN,
	ACTIVE,					// 열려 있고 표시 가능한 상태
	CLOSED,					// 인스턴스는 유지하지만 화면에서는 숨긴 상태
	PENDING_RELEASE,		// 제거 예약 상태
};

enum class UI_RUNTIME_COMMAND_TYPE
{
	SPAWN,
	RELEASE,
};

// UI Input Owner Stack에서 UI가 맡는 역할
enum class UI_INPUT_OWNER_KIND
{
	SCREEN,				// 중첩되지 않는 기본 화면: ESC 메뉴, NPC 대화창, 별바라기 기본창
	MODAL_FULLSCREEN,	// 아래 Owner를 가리는 전체 화면 UI: 장비창, 상점, 강화창, 별바라기 이동창
	MODAL_OVERLAY,		// 아래 Owner를 유지한 채 위에 표시하는 팝업 UI: 확인/취소 팝업sw
	END					// 유효하지 않은 종류 및 enum 범위 검사
};

struct UI_RUNTIME_COMMAND
{
	UI_RUNTIME_COMMAND_TYPE eType = UI_RUNTIME_COMMAND_TYPE::SPAWN;		
	UI_HANDLE				hUI = {};									
};

// 하나의 에셋에서 실제 생성된 런타임 UI
struct UI_RUNTIME_INSTANCE
{
	string				strAssetID;
	_wstring			strInstanceTag;

	// 런타임에서 레이어 단위 표시 제어에 사용
	string              strLayer = "HUD";

	UI_INSTANCE_POLICY	eInstancePolicy = UI_INSTANCE_POLICY::SINGLETON;
	UI_LIFETIME			eLifetime = UI_LIFETIME::GAMEPLAY;
	UI_RUNTIME_STATE	eState = UI_RUNTIME_STATE::ACTIVE;

	_wstring			strControllerTag;

	CUIObject*			pUIObject = { nullptr };
	CUIControllerBase*	pController = { nullptr };
};

// UI Handle이 가리키는 런타임 저장 슬롯
struct UI_RUNTIME_SLOT
{
	UI_RUNTIME_INSTANCE		Instance = {};

	// 새 슬롯의 첫 유효 세대는 1
	_uint					iGeneration = { 1 };
	_bool					bOccupied = { false };
};

// 컨트롤러 팩토리에 전달되는 공통 런타임 정보
struct UI_CONTROLLER_CREATE_CONTEXT
{
	// 컨트롤러를 생성할 UI 에셋
	string			strAssetID;

	// Json 에셋에 기록된 컨트롤러 타입
	string			strControllerType;

	// 싱글톤처럼 특정 인스턴스에 연결되는 경우 사용
	UI_HANDLE		hUI = {};

	// 몬스터 HUD처럼 여러 인스턴스를 생성하는 공유 컨트롤러가 사용할 인스턴스 Prefix
	_wstring		strInstancePrefix;
};

using UI_CONTROLLER_FACTORY = function<HRESULT(const UI_CONTROLLER_CREATE_CONTEXT& Context)>;

// Button의 Focus 대상이 변경됐을 때 Controller에 전달
struct UI_FOCUS_EVENT
{
	// 어느 UI 인스턴스에서 발생했는가
	UI_HANDLE	hUI = {};

	// 원본 Asset의 Element ID
	_uint		iElementID = { UINT_MAX };

	// Component Namespace까지 포함한 실제 런타임 태그
	_wstring	strRuntimeElementTag;

	// 설명창 종류와 선택 그룹을 구분
	string		strFocusGroup;

	// 정적인 메뉴 Button의 의미를 구분
	string		strCommandKey;

	// true: Focus 획득, false: Focus 해제
	_bool		bFocused = { false };
};

// Button 입력이 성공했을 때 Controller에 전달되는 명령
struct UI_COMMAND_EVENT
{
	// 어느 UI 인스턴스에서 발생했는가
	UI_HANDLE hUI = {};

	// 원본 Asset의 Element ID
	_uint iElementID = { UINT_MAX };

	// Component Namespace까지 포함한 실제 런타임 태그
	_wstring strRuntimeElementTag;

	// Controller가 해석할 의미적 명령
	string strCommandKey;
};

NS_END
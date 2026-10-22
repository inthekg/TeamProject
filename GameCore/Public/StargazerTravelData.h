#pragma once

#include "Core_Defines.h"

NS_BEGIN(GameCore)

static constexpr _uint64 INVALID_STARGAZER_ID = static_cast<_uint64>(-1);

// Chapter가 Unload되어도 플레이 세션 동안 유지되는 Runtime 데이터다.
struct STARGAZER_RUNTIME_DATA
{
	_uint64 iStargazerID = INVALID_STARGAZER_ID;
	_uint iChapterIndex = UINT_MAX;
	_uint iSpawnID = UINT_MAX;
	_uint iSectorID = UINT_MAX;

	_wstring strChapterName = {};
	// 추가
	_wstring strLocationName = {};
	string strPreviewTexturePath = {};
	_bool bIsHotel = false;

	_bool bActivated = false;
	_bool bSectorVisited = false;
	_bool bHasRespawnPoint = false;
	Respawn_Point RespawnPoint = {};
};

// UI에 전달하는 정보다. 실제 Teleport 위치는 포함하지 않는다.
struct STARGAZER_TRAVEL_ENTRY
{
	_uint64 iStargazerID = INVALID_STARGAZER_ID;
	_uint iChapterIndex = UINT_MAX;
	_uint iSpawnID = UINT_MAX;
	_uint iSectorID = UINT_MAX;

	_wstring strChapterName = {};
	_wstring strLocationName = {};
	string strPreviewTexturePath = {};
};

// Stargazer 기본 UI가 열릴 때 전달하는 복사형 Event 데이터다.
struct STARGAZER_UI_OPEN_DESC : public Event_Desc
{
	_uint64 iSourceStargazerID = INVALID_STARGAZER_ID;
	vector<STARGAZER_TRAVEL_ENTRY> TravelEntries = {};

	// 추가
	_bool bHotelShortcutAvailable = false;
	_uint64 iHotelStargazerID = INVALID_STARGAZER_ID;
	string strHotelPreviewTexturePath = {}; // 호텔 고정 버튼 선택 시 사용할 프리뷰
};

// UI가 실제 Teleport 목적지를 선택했을 때 보내는 Event 데이터다.
struct STARGAZER_TRAVEL_EXECUTE_DESC : public Event_Desc
{
	_uint64 iTargetStargazerID = INVALID_STARGAZER_ID;
};

NS_END

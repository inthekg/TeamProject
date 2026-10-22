#pragma once
#include "Core_Defines.h"

NS_BEGIN(GameCore)

// 대화 종료 후 실행할 NPC 서비스 종류
enum class NPC_DIALOGUE_SERVICE_TYPE
{
	NONE,				// 실행할 서비스가 없음
	SHOP_BUY,			// 상점 구매 화면
	LEGION_ARM_UPGRADE, // 리전 암 강화
	DONKEY_GATE_ANSWER,	// 문지기 검사 답변
	END					// enum 범위 검사
};

// 현재 NPC 대화 UI가 한 번에 표시할 수 있는 최대 서비스 선택지 개수
constexpr _uint NPC_DIALOGUE_MAX_SERVICE_COUNT = 3;

// 대사 한 줄의 표시 데이터
struct NPC_DIALOGUE_LINE_DESC
{
	_wstring	strSpeakerName;		// 현재 줄을 말하는 화자 이름
	_wstring	strDialogueText;	// 대화 UI에 출력할 문장
	_int		iEventIndex = { -1 };
};

// 서비스 선택지 하나의 표시 및 실행 데이터
struct NPC_DIALOGUE_SERVICE_OPTION_DESC
{
	_wstring					strLabel;											// 서비스 버튼에 출력할 문자열
	NPC_DIALOGUE_SERVICE_TYPE	eServiceType = { NPC_DIALOGUE_SERVICE_TYPE::NONE };	// 선택 시 실행할 서비스
};

// 하나의 DialogueID로 조회되는 전체 선형 대사 스크립트
struct NPC_DIALOGUE_SCRIPT_DESC
{
	string										strDialogueID;		// 스크립트를 조회할 고유 식별자
	vector<NPC_DIALOGUE_LINE_DESC>				DialogueLines;		// 순서대로 출력할 대사 목록
	vector<NPC_DIALOGUE_SERVICE_OPTION_DESC>	ServiceOptions;		// 마지막 대사 뒤에 표시할 서비스 목록
};

// 대화창에서 선택한 서비스를 실제 서비스 UI에 요청할 때 전달하는 데이터
typedef struct tagNPCDialogueServiceRequestDesc final : public Engine::Event_Desc
{
	_uint						iTargetSpawnID = { UINT_MAX };						// 서비스를 요청한 NPC의 맵 Spawn ID
	NPC_TYPE					eNPCType = { NPC_TYPE::END };						// NPC 종류
	NPC_DIALOGUE_SERVICE_TYPE	eServiceType = { NPC_DIALOGUE_SERVICE_TYPE::NONE };	// 실행할 서비스 종류
} NPC_DIALOGUE_SERVICE_REQUEST_DESC;

NS_END
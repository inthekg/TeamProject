#pragma once
#include "DialogueData.h"

NS_BEGIN(GameCore)

// DialogueID를 키로 NPC 대사 스크립트를 보관하고 조회하는 순수 데이터 저장소
class CNPCDialogueRepository final
{
public:
	CNPCDialogueRepository() = default;
	~CNPCDialogueRepository() = default;

public:
	HRESULT Initialize(const _wstring& strFilePath);

	// DialogueID에 해당하는 스크립트를 반환하며, 존재하지 않으면 nullptr을 반환
	const NPC_DIALOGUE_SCRIPT_DESC*						Find_Script(const string& strDialogueID) const;

	// 현재 저장된 스크립트 개수를 반환
	size_t												Get_ScriptCount() const { return m_DialogueScripts.size(); }

private:
	HRESULT												Load_Scripts(const _wstring& strFilePath);
	HRESULT												Parse_Script(const json& ScriptJson, NPC_DIALOGUE_SCRIPT_DESC* pOutScript) const;
	HRESULT												Register_Script(unordered_map<string, NPC_DIALOGUE_SCRIPT_DESC>& DialogueScripts, NPC_DIALOGUE_SCRIPT_DESC Script) const;
	NPC_DIALOGUE_SERVICE_TYPE							Parse_ServiceType(const string& strServiceType) const;
	_bool												Validate_Script(const NPC_DIALOGUE_SCRIPT_DESC& Script) const;

private:
	unordered_map<string, NPC_DIALOGUE_SCRIPT_DESC>		m_DialogueScripts;
};

NS_END
#include "NPCDialogueRepository.h"
#include "Engine_Utility.h"

HRESULT CNPCDialogueRepository::Initialize(const _wstring& strFilePath)
{
	// 같은 저장소를 중복 초기화하면 동일 DialogueID가 다시 등록될 수 있으므로 차단
	if (!m_DialogueScripts.empty() || strFilePath.empty())
		return E_FAIL;

	return Load_Scripts(strFilePath);
}

const NPC_DIALOGUE_SCRIPT_DESC* CNPCDialogueRepository::Find_Script(const string& strDialogueID) const
{
	if (strDialogueID.empty())
		return nullptr;

	const auto iter = m_DialogueScripts.find(strDialogueID);
	if (m_DialogueScripts.end() == iter)
		return nullptr;

	return &iter->second;
}

// ID, 대사 줄, 서비스 옵션 등 필수 데이터가 유효한지 검사
_bool CNPCDialogueRepository::Validate_Script(const NPC_DIALOGUE_SCRIPT_DESC& Script) const
{
	if (Script.strDialogueID.empty())
		return false;

	// 현재 구조에서 대사 줄이 없는 스크립트는 허용하지 않는다.
	if (Script.DialogueLines.empty())
		return false;

	for (const NPC_DIALOGUE_LINE_DESC& Line : Script.DialogueLines)
	{
		if (Line.strSpeakerName.empty() || Line.strDialogueText.empty())
			return false;
	}

	// 현재 NPC 대화 UI가 표시할 수 있는 최대 서비스 개수를 초과하면 등록하지 않는다.
	if (Script.ServiceOptions.size() > NPC_DIALOGUE_MAX_SERVICE_COUNT)
		return false;

	for (const NPC_DIALOGUE_SERVICE_OPTION_DESC& Option : Script.ServiceOptions)
	{
		if (Option.strLabel.empty())
			return false;

		if (Option.eServiceType == NPC_DIALOGUE_SERVICE_TYPE::NONE || Option.eServiceType == NPC_DIALOGUE_SERVICE_TYPE::END)
			return false;
	}

	return true;
}

HRESULT CNPCDialogueRepository::Load_Scripts(const _wstring& strFilePath)
{
	ifstream File(strFilePath, ios::binary);

	if (false == File.is_open())
		return E_FAIL;

	try
	{
		json Root{};
		File >> Root;

		if (false == Root.contains("DialogueScripts") || false == Root["DialogueScripts"].is_array())
			return E_FAIL;

		unordered_map<string, NPC_DIALOGUE_SCRIPT_DESC> LoadedScripts;

		for (const json& ScriptJson : Root["DialogueScripts"])
		{
			NPC_DIALOGUE_SCRIPT_DESC Script{};

			// JSON 하나를 런타임 대화 구조체로 변환한다.
			if (FAILED(Parse_Script(ScriptJson, &Script)))
				return E_FAIL;

			// 중복 ID와 잘못된 대사 데이터를 검사하며 임시 저장소에 등록한다.
			if (FAILED(Register_Script(LoadedScripts, move(Script))))
				return E_FAIL;
		}

		if (LoadedScripts.empty())
			return E_FAIL;

		// 전체 로드가 성공한 경우에만 실제 Repository 데이터와 교체한다.
		m_DialogueScripts.swap(LoadedScripts);
	}
	catch (const json::exception&)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CNPCDialogueRepository::Parse_Script(const json& ScriptJson, NPC_DIALOGUE_SCRIPT_DESC* pOutScript) const
{
	if (pOutScript == nullptr || !ScriptJson.is_object())
		return E_FAIL;

	NPC_DIALOGUE_SCRIPT_DESC Script{};
	Script.strDialogueID = ScriptJson.value("DialogueID", string());

	if (!ScriptJson.contains("DialogueLines") || !ScriptJson["DialogueLines"].is_array())
		return E_FAIL;

	for (const json& LineJson : ScriptJson["DialogueLines"])
	{
		if (!LineJson.is_object())
			return E_FAIL;

		NPC_DIALOGUE_LINE_DESC Line{};

		Line.strSpeakerName = StringToWString(LineJson.value("SpeakerName", string()));
		Line.strDialogueText = StringToWString(LineJson.value("DialogueText", string()));
		Line.iEventIndex = LineJson.value("DialogueEventIndex", -1);

		Script.DialogueLines.push_back(move(Line));
	}

	if (ScriptJson.contains("ServiceOptions"))
	{
		if (!ScriptJson["ServiceOptions"].is_array())
			return E_FAIL;

		for (const json& OptionJson : ScriptJson["ServiceOptions"])
		{
			if (!OptionJson.is_object())
				return E_FAIL;

			NPC_DIALOGUE_SERVICE_OPTION_DESC Option{};

			Option.strLabel = Engine::StringToWString(OptionJson.value("Label", string()));
			Option.eServiceType = Parse_ServiceType(OptionJson.value("ServiceType", string()));

			Script.ServiceOptions.push_back(move(Option));
		}
	}

	*pOutScript = move(Script);

	return S_OK;
}

HRESULT CNPCDialogueRepository::Register_Script(unordered_map<string, NPC_DIALOGUE_SCRIPT_DESC>& DialogueScripts, NPC_DIALOGUE_SCRIPT_DESC Script) const
{
	if (!Validate_Script(Script))
		return E_FAIL;

	const string strDialogueID = Script.strDialogueID;

	// 동일 DialogueID가 있으면 기존 데이터를 덮어쓰지 않고 로드 자체를 실패시킨다.
	if (false == DialogueScripts.emplace(strDialogueID, move(Script)).second)
		return E_FAIL;

	return S_OK;
}

NPC_DIALOGUE_SERVICE_TYPE CNPCDialogueRepository::Parse_ServiceType(const string& strServiceType) const
{
	if (strServiceType == "SHOP_BUY")
		return NPC_DIALOGUE_SERVICE_TYPE::SHOP_BUY;

	if (strServiceType == "LEGION_ARM_UPGRADE")
		return NPC_DIALOGUE_SERVICE_TYPE::LEGION_ARM_UPGRADE;

	if (strServiceType == "DONKEY_GATE_ANSWER")
		return NPC_DIALOGUE_SERVICE_TYPE::DONKEY_GATE_ANSWER;

	// 알 수 없는 문자열은 Validate_Script()에서 실패하도록 END를 반환한다.
	return NPC_DIALOGUE_SERVICE_TYPE::END;
}
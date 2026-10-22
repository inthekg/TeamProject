#include "Animator.h"
#include "Model.h"
#include "Animation.h"
#include "GameInstance.h"
#include "Parser.h"
#include "Channel.h"
#include "AnimationLoader.h"

void CAnimator::Clear_RawAnimations()
{
	for (CAnimation*& pAnimaion : m_Animations)
		Safe_Release(pAnimaion);

	m_Animations.clear();
}

void CAnimator::Resolve_RawAnimationIndices(ANIMATION_EDIT_DATA& EditData) const
{
	unordered_map<_uint64, _uint> RawIndexByHash;
	RawIndexByHash.reserve(m_Animations.size());

	for (_uint i = 0; i < m_Animations.size(); i++)
	{
		CAnimation* pAnimation = m_Animations[i];

		if (pAnimation == nullptr)
			continue;

		RawIndexByHash[pAnimation->Get_AnimationFileHash()] = i;
	}

	for (ANIM_CLIP_SETTING& Clip : EditData.ClipSettings)
	{
		Clip.iRawAnimationIndex = INVALID_ANIMATION_INDEX;

		if (Clip.iRawAnimationHash == 0 && !Clip.strSourceName.empty())
			Clip.iRawAnimationHash = Hash_String(StringToWString(Clip.strSourceName).c_str());

		auto iter = RawIndexByHash.find(Clip.iRawAnimationHash);
		if (iter != RawIndexByHash.end())
			Clip.iRawAnimationIndex = iter->second;
	}
}

_bool CAnimator::Is_ValidRawAnimationIndex(_uint iRawAnimationIndex) const
{
	return iRawAnimationIndex != INVALID_ANIMATION_INDEX && iRawAnimationIndex < m_Animations.size() && m_Animations[iRawAnimationIndex] != nullptr;
}

HRESULT CAnimator::Ready_RawAnimations(const RAW_ANIMATION_LOAD_DESC& Desc)
{
	Clear_RawAnimations();

	if (Desc.strFolderName.empty() || Desc.strAnimationBaseName.empty())
		return E_FAIL;

	// 1차 : 로더가 미리 등록해 둔 애니메이션을 리소스 매니저에서 가져온다.
	if (SUCCEEDED(Ready_RawAnimations_FromResourceManager(Desc)))
		return S_OK;

	// 2차 : 에디터 프리뷰 또는 선등록 누락 시 여기서 리소스 매니저에 등록
	if (FAILED(CAnimationLoader::Ready_RawAnimations(
		m_pGameInstance,
		Desc.strFolderName,
		Desc.strAnimationBaseName,
		Desc.eTimeUnit,
		Desc.bStaticAnimation)))
	{
		return E_FAIL;
	}

	// 3차 : 방금 등록된 애니메이션들 다시 가져옴
	return Ready_RawAnimations_FromResourceManager(Desc);
}

HRESULT CAnimator::Ready_RawAnimations_FromResourceManager(const RAW_ANIMATION_LOAD_DESC& Desc)
{
	if (m_pGameInstance == nullptr)
		return E_FAIL;

	const _wstring strResourceDir = m_pGameInstance->Get_ResourcePath() + L"/Models/" + Desc.strFolderName + L"/";

	const _wstring strCSVPath = strResourceDir + Desc.strAnimationBaseName + GlobalFileRoute::g_strAnimationCSVFileTag;

	vector<AnimationReadData> AnimDatas;

	const bool bResult = CSVParser<AnimationReadData>::CSV_Parse(AnimDatas, strCSVPath, [](const vector<_wstring>& tokens)
		{
			return AnimationReadData
			{
				// tokens[0]은 CSV 순번이라 사용하지 않는다.
				tokens[1],                          // AnimName
				tokens[2],                          // AnimationFileName
				stof(tokens[3]),                    // Duration
				stof(tokens[4]),                    // TickPerSecond
				(tokens[5] == L"0" ? false : true), // Loop
				stof(tokens[6]),                    // BlendTime
				(tokens[7] == L"0" ? false : true)  // RootMotion
			};
		});

	if (!bResult || AnimDatas.empty())
		return E_FAIL;

	vector<CAnimation*> FoundAnimations;
	FoundAnimations.reserve(AnimDatas.size());

	for (const AnimationReadData& AnimData : AnimDatas)
	{
		const _uint64 iAnimationKey = Hash_String((Desc.strFolderName + L"/" + AnimData.strAnimationFileName).c_str());

		CAnimation* pAnimation = nullptr;

		if (FAILED(m_pGameInstance->Get_Animation(iAnimationKey, &pAnimation)))
		{
			// 일부만 찾은 상태로 m_Animations에 넣으면 인덱스가 꼬일 수 있으므로 전부 폐기
			for (CAnimation*& pFoundAnimation : FoundAnimations)
				Safe_Release(pFoundAnimation);

			FoundAnimations.clear();
			return E_FAIL;
		}

		// Get_Animation에서 AddRef 된 포인터
		FoundAnimations.push_back(pAnimation);
	}

	m_Animations = std::move(FoundAnimations);

	return m_Animations.empty() ? E_FAIL : S_OK;
}

void CAnimator::Update_RawTime(_float fTimeDelta)
{
	if (m_iCurRawAnimationIndex < 0 || m_iCurRawAnimationIndex >= m_Animations.size())
	{
		m_bPlaying = false;
		return;
	}

	CAnimation* pAnimation = m_Animations[m_iCurRawAnimationIndex];
	if (pAnimation == nullptr)
	{
		m_bPlaying = false;
		return;
	}

	const _float fDuration = pAnimation->Get_Duration();

	m_fRawTime += fTimeDelta * pAnimation->Get_TickPerSecond();

	if (m_fRawTime >= fDuration)
	{
		if (m_bRawLoop)
			m_fRawTime = fmodf(m_fRawTime, fDuration);
		else
		{
			m_fRawTime = fDuration;
			m_bPlaying = false;
			m_bEnded = true;
		}
	}
}

void CAnimator::Update_RawPose()
{
	if (m_iCurRawAnimationIndex < 0 || m_iCurRawAnimationIndex >= m_Animations.size())
	{
		m_CurPose = m_BasePose;
		return;
	}

	CAnimation* pAnimation = m_Animations[m_iCurRawAnimationIndex];
	if (pAnimation == nullptr)
	{
		m_CurPose = m_BasePose;
		return;
	}

	m_CurPose = m_BasePose;

	if (m_KeyFrameIndices.size() != m_CurPose.size())
		m_KeyFrameIndices.resize(m_CurPose.size());

	pAnimation->Get_Pose(m_CurPose, m_fRawTime, m_KeyFrameIndices);
}

HRESULT CAnimator::Try_LoadEditData_FromFile(const RAW_ANIMATION_LOAD_DESC& Desc)
{
	if (Desc.strFolderName.empty() || Desc.strAnimationBaseName.empty())
		return S_FALSE;

	_wstring strPath = m_pGameInstance->Get_ResourcePath();
	strPath += L"\\Models\\";
	strPath += Desc.strFolderName;
	strPath += L"\\";
	strPath += Desc.strFolderName;
	strPath += GlobalFileRoute::g_strAnimSettingFileTag;

	ifstream TestFile(strPath);
	if (!TestFile.is_open())
		return S_FALSE;
	TestFile.close();


	ANIMATION_EDIT_DATA EditData = {};
	if (FAILED(Parse_AnimationEditDataJson(strPath, EditData)))
		return E_FAIL;

	Set_EditData(EditData);

	return S_OK;
}

HRESULT CAnimator::Parse_AnimationEditDataJson(const _wstring& strFilePath, ANIMATION_EDIT_DATA& OutEditData)
{
	ifstream File(strFilePath);
	if (!File.is_open())
		return E_FAIL;

	json Root = {};
	File >> Root;

	OutEditData = {};

	if (FAILED(Parse_ClipSettingJson(Root, OutEditData.ClipSettings)))
		return E_FAIL;

	if (FAILED(Parse_SequencesJson(Root, OutEditData.Sequences)))
		return E_FAIL;

	if (FAILED(Parse_ControllerJson(Root, OutEditData.Controller)))
		return E_FAIL;

	return S_OK;
}

HRESULT CAnimator::Parse_ClipSettingJson(const json& Root, vector<ANIM_CLIP_SETTING>& OutClipSettings)
{
	OutClipSettings.clear();

	if (!Root.contains("Clips") || !Root["Clips"].is_array())
		return S_OK;

	const json& Clips = Root["Clips"];

	for (const auto& ClipJson : Clips)
	{
		ANIM_CLIP_SETTING Clip = {};

		Clip.strSourceName = ClipJson.value("SourceName", "");
		Clip.strExportName = ClipJson.value("ExportName", Clip.strSourceName);

		Clip.iClipHash = ClipJson.value("ClipHash", 0ull);
		Clip.iRawAnimationHash = ClipJson.value("RawAnimationHash", 0ull);
		Clip.iRawAnimationIndex = ClipJson.value("RawAnimationIndex", -1);

		Clip.fSourceDuration = ClipJson.value("SourceDuration", 0.f);
		Clip.iSourceFrameCount = ClipJson.value("SourceFrameCount", 0);

		Clip.iStartFrame = ClipJson.value("StartFrame", 0);
		Clip.iEndFrame = ClipJson.value("EndFrame", Clip.iSourceFrameCount > 0 ? Clip.iSourceFrameCount - 1 : 0);

		Clip.bLoop = ClipJson.value("Loop", false);
		Clip.bUseRootMotion = ClipJson.value("UseRootMotion", false);
		Clip.bReverse = ClipJson.value("Reverse", false);

		Clip.fPlayRate = ClipJson.value("PlayRate", 1.f);
		Clip.fBlendInTime = ClipJson.value("BlendInTime", 0.f);
		Clip.fBlendOutTime = ClipJson.value("BlendOutTime", 0.f);

		if (Clip.iClipHash == 0 && !Clip.strExportName.empty())
			Clip.iClipHash = Hash_String(StringToWString(Clip.strExportName).c_str());

		if (Clip.iRawAnimationHash == 0 && !Clip.strSourceName.empty())
			Clip.iRawAnimationHash = Hash_String(StringToWString(Clip.strSourceName).c_str());


		// SpeedSections
		if (ClipJson.contains("SpeedSections") && ClipJson["SpeedSections"].is_array())
		{
			for (const auto& SectionJson : ClipJson["SpeedSections"])
			{
				ANIM_SPEED_SECTION Section = {};

				Section.iStartFrame = SectionJson.value("StartFrame", 0);
				Section.iEndFrame = SectionJson.value("EndFrame", 0);
				Section.fPlayRate = SectionJson.value("PlayRate", 1.f);

				Clip.SpeedSections.push_back(Section);
			}
		}

		// Notifies
		if (ClipJson.contains("Notifies") && ClipJson["Notifies"].is_array())
		{
			for (const auto& NotifyJson : ClipJson["Notifies"])
			{
				ANIM_NOTIFY Notify = {};

				Notify.strName = NotifyJson.value("Name", "");
				Notify.iNameHash = NotifyJson.value("NameHash", 0ull);

				if (Notify.iNameHash == 0 && !Notify.strName.empty())
					Notify.iNameHash = Hash_String(StringToWString(Notify.strName).c_str());

				const _uint iType = NotifyJson.value("Type", static_cast<_uint>(ANIM_NOTIFY_TYPE::CUSTOM));
				Notify.eType = iType < static_cast<_uint>(ANIM_NOTIFY_TYPE::END) ? static_cast<ANIM_NOTIFY_TYPE>(iType) : ANIM_NOTIFY_TYPE::CUSTOM;

				Notify.iFrame = NotifyJson.value("Frame", 0);
				Notify.fTime = NotifyJson.value("Time", 0.f);

				Notify.strPayload = NotifyJson.value("Payload", "");
				Notify.iPayloadHash = NotifyJson.value("PayloadHash", 0ull);

				if (Notify.iPayloadHash == 0 && !Notify.strPayload.empty())
					Notify.iPayloadHash = Hash_String(StringToWString(Notify.strPayload).c_str());

				Notify.bUseSlot = NotifyJson.value("UseSlot", false);

				if (Notify.bUseSlot && NotifyJson.contains("Slot") && NotifyJson["Slot"].is_object())
				{
					const json& SlotJson = NotifyJson["Slot"];

					Notify.Slot.strSlotName = SlotJson.value("Name", "");
					Notify.Slot.iSlotHash = SlotJson.value("NameHash", 0ull);

					if (Notify.Slot.iSlotHash == 0 && !Notify.Slot.strSlotName.empty())
						Notify.Slot.iSlotHash = Hash_String(StringToWString(Notify.Slot.strSlotName).c_str());

					Notify.Slot.strBoneName = SlotJson.value("Bone", "");
					Notify.Slot.iBoneHash = SlotJson.value("BoneHash", 0ull);
					Notify.Slot.iBoneIndex = SlotJson.value("BoneIndex", -1);

					if (Notify.Slot.iBoneHash == 0 && !Notify.Slot.strBoneName.empty())
						Notify.Slot.iBoneHash = Hash_String(StringToWString(Notify.Slot.strBoneName).c_str());

					Notify.Slot.vOffsetScale = { 1.f,1.f, 1.f };

					if (SlotJson.contains("OffsetPosition") && SlotJson["OffsetPosition"].is_array() && SlotJson["OffsetPosition"].size() >= 3)
					{
						Notify.Slot.vOffsetPosition.x = SlotJson["OffsetPosition"][0].get<_float>();
						Notify.Slot.vOffsetPosition.y = SlotJson["OffsetPosition"][1].get<_float>();
						Notify.Slot.vOffsetPosition.z = SlotJson["OffsetPosition"][2].get<_float>();
					}
					if (SlotJson.contains("OffsetRotation") && SlotJson["OffsetRotation"].is_array() && SlotJson["OffsetRotation"].size() >= 3)
					{
						Notify.Slot.vOffsetRotation.x = SlotJson["OffsetRotation"][0].get<_float>();
						Notify.Slot.vOffsetRotation.y = SlotJson["OffsetRotation"][1].get<_float>();
						Notify.Slot.vOffsetRotation.z = SlotJson["OffsetRotation"][2].get<_float>();
					}
					if (SlotJson.contains("OffsetScale") && SlotJson["OffsetScale"].is_array() && SlotJson["OffsetScale"].size() >= 3)
					{
						Notify.Slot.vOffsetScale.x = SlotJson["OffsetScale"][0].get<_float>();
						Notify.Slot.vOffsetScale.y = SlotJson["OffsetScale"][1].get<_float>();
						Notify.Slot.vOffsetScale.z = SlotJson["OffsetScale"][2].get<_float>();
					}
				}

				Clip.Notifies.push_back(Notify);
			}
		}

		// Windows
		if (ClipJson.contains("Windows") && ClipJson["Windows"].is_array())
		{
			for (const auto& WindowJson : ClipJson["Windows"])
			{
				ANIM_WINDOW Window{};

				Window.strName = WindowJson.value("Name", "");
				Window.iNameHash = WindowJson.value("NameHash", 0ull);

				if (Window.iNameHash == 0 && !Window.strName.empty())
					Window.iNameHash = Hash_String(StringToWString(Window.strName).c_str());

				const _uint iType = WindowJson.value("Type", static_cast<_uint>(ANIM_WINDOW_TYPE::CUSTOM));
				Window.eType = iType < static_cast<_uint>(ANIM_WINDOW_TYPE::END) ? static_cast<ANIM_WINDOW_TYPE>(iType) : ANIM_WINDOW_TYPE::CUSTOM;

				Window.iStartFrame = WindowJson.value("StartFrame", 0);
				Window.iEndFrame = WindowJson.value("EndFrame", 0);
				Window.fValue = WindowJson.value("Value", 0.f);

				Window.iPayloadHash = WindowJson.value("PayloadHash", 0ull);
				Window.strPayload = WindowJson.value("Payload", "");

				if (Window.iPayloadHash == 0 && !Window.strPayload.empty())
					Window.iPayloadHash = Hash_String(StringToWString(Window.strPayload).c_str());

				Clip.Windows.push_back(Window);
			}
		}

		OutClipSettings.push_back(Clip);
	}

	return S_OK;
}

HRESULT CAnimator::Parse_SequencesJson(const json& Root, vector<ANIM_SEQUENCE>& OutSequences)
{
	OutSequences.clear();

	if (!Root.contains("Sequences") || !Root["Sequences"].is_array())
		return S_OK;

	for (const json& SequenceJson : Root["Sequences"])
	{
		ANIM_SEQUENCE Sequence = {};

		Sequence.strName = SequenceJson.value("Name", "");
		Sequence.iNameHash = SequenceJson.value("NameHash", 0ull);

		if (Sequence.iNameHash == 0 && !Sequence.strName.empty())
			Sequence.iNameHash = Hash_String(StringToWString(Sequence.strName).c_str());

		const _uint iUsage = SequenceJson.value("Usage", static_cast<_uint>(ANIM_SEQUENCE_USAGE::BASE));
		Sequence.eUsage = iUsage < static_cast<_uint>(ANIM_SEQUENCE_USAGE::END) ? static_cast<ANIM_SEQUENCE_USAGE>(iUsage) : ANIM_SEQUENCE_USAGE::BASE;

		const _uint iFlow = SequenceJson.value("Flow", static_cast<_uint>(ANIM_SEQUENCE_FLOW::SUSTAINED_LOOP));
		Sequence.eFlow = iFlow < static_cast<_uint>(ANIM_SEQUENCE_FLOW::END) ? static_cast<ANIM_SEQUENCE_FLOW>(iFlow) : ANIM_SEQUENCE_FLOW::SUSTAINED_LOOP;

		const _uint iStepBlendMode = SequenceJson.value("StepBlendMode", static_cast<_uint>(ANIM_SEQUENCE_STEP_BLEND_MODE::ALL_STEPS));
		Sequence.eStepBlendMode = iStepBlendMode < static_cast<_uint>(ANIM_SEQUENCE_STEP_BLEND_MODE::END) ? static_cast<ANIM_SEQUENCE_STEP_BLEND_MODE>(iStepBlendMode) : ANIM_SEQUENCE_STEP_BLEND_MODE::ALL_STEPS;

		const _uint iRestartRule = SequenceJson.value("RestartRule", static_cast<_uint>(ANIM_SEQUENCE_RESTART_RULE::ANY_TIME));
		Sequence.eRestartRule = iRestartRule < static_cast<_uint>(ANIM_SEQUENCE_RESTART_RULE::END) ? static_cast<ANIM_SEQUENCE_RESTART_RULE>(iRestartRule) : ANIM_SEQUENCE_RESTART_RULE::ANY_TIME;

		Sequence.bStartCancelByWindow = SequenceJson.value("StartCancelByWindow", true);
		Sequence.bAllowEndToLoopReentry = SequenceJson.value("AllowEndToLoopReentry", true);
		Sequence.bUseCommitTime = SequenceJson.value("UseCommitTime", false);
		Sequence.fCommitTime = SequenceJson.value("CommitTime", 0.f);
		Sequence.fLoopToEndBlendTime = SequenceJson.value("LoopToEndBlendTime", 0.f);
		Sequence.fEndBlendOutTime = SequenceJson.value("EndBlendOutTime", 0.f);

		Sequence.fReentryBlendTime = SequenceJson.value("ReentryBlendTime", 0.f);
		Sequence.fReentryStartOffsetTime = SequenceJson.value("ReentryStartOffsetTime", 0.f);

		if (Sequence.fReentryBlendTime < 0.f)
			Sequence.fReentryBlendTime = 0.f;

		if (Sequence.fReentryStartOffsetTime < 0.f)
			Sequence.fReentryStartOffsetTime = 0.f;

		if (SequenceJson.contains("Steps") && SequenceJson["Steps"].is_array())
		{
			for (const json& StepJson : SequenceJson["Steps"])
			{
				ANIM_SEQUENCE_STEP Step = {};

				const _uint iStepType = StepJson.value("StepType", static_cast<_uint>(ANIM_SEQUENCE_STEP_TYPE::ACTIVE));
				Step.eStepType = iStepType < static_cast<_uint>(ANIM_SEQUENCE_STEP_TYPE::END_ENUM) ? static_cast<ANIM_SEQUENCE_STEP_TYPE>(iStepType) : ANIM_SEQUENCE_STEP_TYPE::ACTIVE;

				Step.iClipHash = StepJson.value("ClipHash", 0ull);
				Step.iClipIndex = -1;

				if (StepJson.contains("HoldConditions") && StepJson["HoldConditions"].is_array())
				{
					for (const json& ConditionJson : StepJson["HoldConditions"])
					{
						ANIM_CONTROLLER_CONDITION Condition = {};

						Condition.iParameterHash = ConditionJson.value("ParameterHash", 0ull);
						Condition.iParameterIndex = -1;

						const _uint iCompareType = ConditionJson.value("CompareType", static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::True));

						Condition.eCompareType = iCompareType < static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::END) ? static_cast<ANIM_CONTROLLER_COMPARE_TYPE>(iCompareType) : ANIM_CONTROLLER_COMPARE_TYPE::True;

						Condition.bBoolValue = ConditionJson.value("BoolValue", false);
						Condition.iIntValue = ConditionJson.value("IntValue", 0);
						Condition.fFloatValue = ConditionJson.value("FloatValue", 0.f);

						Step.HoldConditions.push_back(Condition);
					}
				}

				Sequence.Steps.push_back(Step);
			}
		}

		OutSequences.push_back(Sequence);
	}

	return S_OK;
}

HRESULT CAnimator::Parse_ControllerJson(const json& Root, ANIM_CONTROLLER& OutController)
{
	OutController = {};

	if (!Root.contains("Controller") || !Root["Controller"].is_object())
		return S_OK;

	const json& ControllerJson = Root["Controller"];

	OutController.strName = ControllerJson.value("Name", string("New Controller"));
	OutController.iNameHash = ControllerJson.value("NameHash", 0ull);

	if (OutController.iNameHash == 0 && !OutController.strName.empty())
		OutController.iNameHash = Hash_String(StringToWString(OutController.strName).c_str());

	OutController.iEntryStateHash = ControllerJson.value("EntryStateHash", 0ull);
	OutController.iEntryStateIndex = -1;

	if (ControllerJson.contains("Parameters") && ControllerJson["Parameters"].is_array())
	{
		for (const json& ParamJson : ControllerJson["Parameters"])
		{
			ANIM_CONTROLLER_PARAMETER Param = {};

			Param.strName = ParamJson.value("Name", "");
			Param.iNameHash = ParamJson.value("NameHash", 0ull);

			if (Param.iNameHash == 0 && !Param.strName.empty())
				Param.iNameHash = Hash_String(StringToWString(Param.strName).c_str());

			const _uint iType = ParamJson.value("Type", static_cast<_uint>(ANIM_CONTROLLER_PARAM_TYPE::BOOL));
			Param.eType = iType < static_cast<_uint>(ANIM_CONTROLLER_PARAM_TYPE::END) ? static_cast<ANIM_CONTROLLER_PARAM_TYPE>(iType) : ANIM_CONTROLLER_PARAM_TYPE::BOOL;

			Param.bDefaultBool = ParamJson.value("DefaultBool", false);
			Param.iDefaultInt = ParamJson.value("DefaultInt", 0);
			Param.fDefaultFloat = ParamJson.value("DefaultFloat", 0.f);

			OutController.Parameters.push_back(Param);
		}
	}

	if (ControllerJson.contains("States") && ControllerJson["States"].is_array())
	{
		for (const json& StateJson : ControllerJson["States"])
		{
			ANIM_CONTROLLER_STATE State = {};

			State.strName = StateJson.value("Name", "");
			State.iNameHash = StateJson.value("NameHash", 0ull);

			if (State.iNameHash == 0 && !State.strName.empty())
				State.iNameHash = Hash_String(StringToWString(State.strName).c_str());

			const _uint iMotionType = StateJson.value("MotionType", static_cast<_uint>(ANIM_CONTROLLER_MOTION_TYPE::NONE));
			State.eMotionType = iMotionType < static_cast<_uint>(ANIM_CONTROLLER_MOTION_TYPE::END) ? static_cast<ANIM_CONTROLLER_MOTION_TYPE>(iMotionType) : ANIM_CONTROLLER_MOTION_TYPE::NONE;

			State.iMotionHash = StateJson.value("MotionHash", 0ull);
			State.iMotionIndex = -1;

			const _uint iEntryMode = StateJson.value("EntryMode", static_cast<_uint>(ANIM_STATE_ENTRY_MODE::AUTO));
			State.eEntryMode = iEntryMode < static_cast<_uint>(ANIM_STATE_ENTRY_MODE::END) ? static_cast<ANIM_STATE_ENTRY_MODE>(iEntryMode) : ANIM_STATE_ENTRY_MODE::AUTO;

			State.eHoldRule = ANIM_STATE_HOLD_RULE::NONE;

			if (StateJson.contains("HoldRule"))
			{
				const _uint iHoldRule = StateJson["HoldRule"].get<_uint>();

				if (iHoldRule < static_cast<_uint>(ANIM_STATE_HOLD_RULE::END))
					State.eHoldRule = static_cast<ANIM_STATE_HOLD_RULE>(iHoldRule);
			}

			const _uint iRequestRule = StateJson.value("RequestRule", static_cast<_uint>(ANIM_STATE_REQUEST_RULE::NEVER));
			State.eRequestRule = iRequestRule < static_cast<_uint>(ANIM_STATE_REQUEST_RULE::END) ? static_cast<ANIM_STATE_REQUEST_RULE>(iRequestRule) : ANIM_STATE_REQUEST_RULE::NEVER;

			State.iPriority = StateJson.value("Priority", 0);
			State.bCanRestart = StateJson.value("CanRestart", false);
			State.bAllowPoseLayers = StateJson.value("AllowPoseLayers", true);

			if (StateJson.contains("Conditions") && StateJson["Conditions"].is_array())
			{
				for (const json& ConditionJson : StateJson["Conditions"])
				{
					ANIM_CONTROLLER_CONDITION Condition = {};

					Condition.iParameterHash = ConditionJson.value("ParameterHash", 0ull);
					Condition.iParameterIndex = -1;

					const _uint iCompareType = ConditionJson.value("CompareType", static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::True));
					Condition.eCompareType = iCompareType < static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::END) ? static_cast<ANIM_CONTROLLER_COMPARE_TYPE>(iCompareType) : ANIM_CONTROLLER_COMPARE_TYPE::True;

					Condition.bBoolValue = ConditionJson.value("BoolValue", false);
					Condition.iIntValue = ConditionJson.value("IntValue", 0);
					Condition.fFloatValue = ConditionJson.value("FloatValue", 0.f);

					State.EnterConditions.push_back(Condition);
				}
			}

			OutController.States.push_back(State);
		}
	}

	if (ControllerJson.contains("PoseLayerRules") && ControllerJson["PoseLayerRules"].is_array())
	{
		for (const json& RuleJson : ControllerJson["PoseLayerRules"])
		{
			ANIM_POSE_LAYER_RULE Rule = {};

			Rule.strName = RuleJson.value("Name", string(""));
			Rule.iNameHash = RuleJson.value("NameHash", 0ull);

			if (Rule.iNameHash == 0 && !Rule.strName.empty())
				Rule.iNameHash = Hash_String(StringToWString(Rule.strName).c_str());

			Rule.bEnable = RuleJson.value("Enable", true);

			const _uint iMotionType = RuleJson.value("MotionType", static_cast<_uint>(ANIM_CONTROLLER_MOTION_TYPE::CLIP));
			Rule.eMotionType = iMotionType < static_cast<_uint>(ANIM_CONTROLLER_MOTION_TYPE::END) ? static_cast<ANIM_CONTROLLER_MOTION_TYPE>(iMotionType) : ANIM_CONTROLLER_MOTION_TYPE::CLIP;

			Rule.iMotionHash = RuleJson.value("MotionHash", RuleJson.value("ClipHash", 0ull));
			Rule.iMotionIndex = -1;


			Rule.strMaskRootBoneName = RuleJson.value("MaskRootBoneName", string(""));
			Rule.iMaskRootBoneHash = RuleJson.value("MaskRootBoneHash", 0ull);
			Rule.iMaskRootBoneIndex = -1;

			if (Rule.iMaskRootBoneHash == 0 && !Rule.strMaskRootBoneName.empty())
				Rule.iMaskRootBoneHash = Hash_String(StringToWString(Rule.strMaskRootBoneName).c_str());

			Rule.eBlendType = static_cast<ANIM_POSE_LAYER_BLEND_TYPE>(RuleJson.value("BlendType", static_cast<_int>(ANIM_POSE_LAYER_BLEND_TYPE::OVERRIDE)));
			Rule.fWeight = RuleJson.value("Weight", 1.f);
			Rule.bLoop = RuleJson.value("Loop", true);
			Rule.fPlayRate = RuleJson.value("PlayRate", 1.f);

			if (Rule.fWeight < 0.f)
				Rule.fWeight = 0.f;
			if (Rule.fWeight > 1.f)
				Rule.fWeight = 1.f;
			if (Rule.fPlayRate <= 0.f)
				Rule.fPlayRate = 1.f;

			if (RuleJson.contains("Conditions") && RuleJson["Conditions"].is_array())
			{
				for (const json& ConditionJson : RuleJson["Conditions"])
				{
					ANIM_CONTROLLER_CONDITION Condition = {};

					Condition.iParameterHash = ConditionJson.value("ParameterHash", 0ull);
					Condition.iParameterIndex = -1;

					const _uint iCompareType = ConditionJson.value("CompareType", static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::True));
					Condition.eCompareType = iCompareType < static_cast<_uint>(ANIM_CONTROLLER_COMPARE_TYPE::END) ?
						static_cast<ANIM_CONTROLLER_COMPARE_TYPE>(iCompareType) :
						ANIM_CONTROLLER_COMPARE_TYPE::True;

					Condition.bBoolValue = ConditionJson.value("BoolValue", false);
					Condition.iIntValue = ConditionJson.value("IntValue", 0);
					Condition.fFloatValue = ConditionJson.value("FloatValue", 0.f);

					Rule.Conditions.push_back(Condition);
				}
			}

			OutController.PoseLayerRules.push_back(Rule);
		}
	}

	OutController.ActionStateMaps.clear();

	if (ControllerJson.contains("ActionStateMaps") && ControllerJson["ActionStateMaps"].is_array())
	{
		for (const json& MapJson : ControllerJson["ActionStateMaps"])
		{
			ANIM_ACTION_STATE_MAP Map = {};

			Map.strActionName = MapJson.value("ActionName", "");
			Map.iActionHash = MapJson.value("ActionHash", 0ull);

			if (Map.iActionHash == 0 && !Map.strActionName.empty())
				Map.iActionHash = Hash_String(StringToWString(Map.strActionName).c_str());

			Map.iStateHash = MapJson.value("StateHash", 0ull);
			Map.iStateIndex = MapJson.value("StateIndex", -1);

			OutController.ActionStateMaps.push_back(Map);
		}
	}

	return S_OK;
}

void CAnimator::Validate_EditData(ANIMATION_EDIT_DATA& EditData)
{
	for (ANIM_CLIP_SETTING& Clip : EditData.ClipSettings)
	{
		if (Clip.strExportName.empty())
			Clip.strExportName = Clip.strSourceName;

		if (Clip.iClipHash == 0 && !Clip.strExportName.empty())
			Clip.iClipHash = Hash_String(StringToWString(Clip.strExportName).c_str());

		if (Clip.iRawAnimationHash == 0 && !Clip.strSourceName.empty())
			Clip.iRawAnimationHash = Hash_String(StringToWString(Clip.strSourceName).c_str());

		if (Clip.fPlayRate <= 0.f)
			Clip.fPlayRate = 1.f;

		if (Clip.iSourceFrameCount > 0)
		{
			const _uint iSourceMaxFrame = Clip.iSourceFrameCount - 1;

			if (Clip.iStartFrame > iSourceMaxFrame)
				Clip.iStartFrame = iSourceMaxFrame;

			if (Clip.iEndFrame > iSourceMaxFrame)
				Clip.iEndFrame = iSourceMaxFrame;

			if (Clip.iStartFrame > Clip.iEndFrame)
				swap(Clip.iStartFrame, Clip.iEndFrame);

			const _uint iLocalMaxFrame = Clip.iEndFrame - Clip.iStartFrame;

			for (ANIM_SPEED_SECTION& Section : Clip.SpeedSections)
			{
				if (Section.fPlayRate <= 0.f)
					Section.fPlayRate = 1.f;

				if (Section.iStartFrame > iLocalMaxFrame)
					Section.iStartFrame = iLocalMaxFrame;

				if (Section.iEndFrame > iLocalMaxFrame)
					Section.iEndFrame = iLocalMaxFrame;

				if (Section.iStartFrame > Section.iEndFrame)
					swap(Section.iStartFrame, Section.iEndFrame);
			}

			for (ANIM_NOTIFY& Notify : Clip.Notifies)
			{
				if (Notify.iFrame > iLocalMaxFrame)
					Notify.iFrame = iLocalMaxFrame;
			}

			for (ANIM_WINDOW& Window : Clip.Windows)
			{
				if (Window.iStartFrame > iLocalMaxFrame)
					Window.iStartFrame = iLocalMaxFrame;

				if (Window.iEndFrame > iLocalMaxFrame)
					Window.iEndFrame = iLocalMaxFrame;

				if (Window.iStartFrame > Window.iEndFrame)
					swap(Window.iStartFrame, Window.iEndFrame);
			}
		}
	}

	for (ANIM_POSE_LAYER_RULE& Rule : EditData.Controller.PoseLayerRules)
	{
		if (Rule.eMotionType != ANIM_CONTROLLER_MOTION_TYPE::CLIP && Rule.eMotionType != ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
			Rule.eMotionType = ANIM_CONTROLLER_MOTION_TYPE::CLIP;

		if (Rule.fWeight < 0.f)
			Rule.fWeight = 0.f;

		if (Rule.fWeight > 1.f)
			Rule.fWeight = 1.f;

		if (Rule.fPlayRate <= 0.f)
			Rule.fPlayRate = 1.f;
	}

	ANIM_CONTROLLER& Controller = EditData.Controller;
}

void CAnimator::Resolve_EditData(ANIMATION_EDIT_DATA& EditData)
{
	unordered_map<_uint64, _int> ClipIndexByHash;
	unordered_map<_uint64, _int> SequenceIndexByHash;
	unordered_map<_uint64, _int> ParamIndexByHash;
	unordered_map<_uint64, _int> StateIndexByHash;

	ClipIndexByHash.reserve(EditData.ClipSettings.size());

	for (_uint i = 0; i < EditData.ClipSettings.size(); i++)
	{
		const _uint64 iHash = EditData.ClipSettings[i].iClipHash;

		if (iHash != 0)
			ClipIndexByHash[iHash] = static_cast<_int>(i);
	}

	SequenceIndexByHash.reserve(EditData.Sequences.size());

	for (_uint i = 0; i < EditData.Sequences.size(); ++i)
	{
		const _uint64 iHash = EditData.Sequences[i].iNameHash;

		if (iHash != 0)
			SequenceIndexByHash[iHash] = static_cast<_int>(i);
	}

	for (ANIM_SEQUENCE& Sequence : EditData.Sequences)
	{
		for (ANIM_SEQUENCE_STEP& Step : Sequence.Steps)
		{
			Step.iClipIndex = -1;

			auto iter = ClipIndexByHash.find(Step.iClipHash);
			if (iter != ClipIndexByHash.end())
				Step.iClipIndex = iter->second;
		}
	}

	ANIM_CONTROLLER& Controller = EditData.Controller;

	ParamIndexByHash.reserve(Controller.Parameters.size());
	StateIndexByHash.reserve(Controller.States.size());

	for (_uint i = 0; i < Controller.Parameters.size(); ++i)
	{
		const _uint64 iHash = Controller.Parameters[i].iNameHash;

		if (iHash != 0)
			ParamIndexByHash[iHash] = static_cast<_int>(i);
	}

	for (_uint i = 0; i < Controller.States.size(); ++i)
	{
		const _uint64 iHash = Controller.States[i].iNameHash;

		if (iHash != 0)
			StateIndexByHash[iHash] = static_cast<_int>(i);
	}

	Controller.iEntryStateIndex = -1;

	auto EntryIter = StateIndexByHash.find(Controller.iEntryStateHash);
	if (EntryIter != StateIndexByHash.end())
		Controller.iEntryStateIndex = EntryIter->second;

	for (ANIM_CONTROLLER_STATE& State : Controller.States)
	{
		State.iMotionIndex = -1;

		if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
		{
			auto iter = ClipIndexByHash.find(State.iMotionHash);
			if (iter != ClipIndexByHash.end())
				State.iMotionIndex = iter->second;
		}
		else if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
		{
			auto iter = SequenceIndexByHash.find(State.iMotionHash);
			if (iter != SequenceIndexByHash.end())
				State.iMotionIndex = iter->second;
		}

		for (ANIM_CONTROLLER_CONDITION& Condition : State.EnterConditions)
		{
			Condition.iParameterIndex = -1;

			auto ParamIter = ParamIndexByHash.find(Condition.iParameterHash);
			if (ParamIter != ParamIndexByHash.end())
				Condition.iParameterIndex = ParamIter->second;
		}
	}

	for (ANIM_ACTION_STATE_MAP& Map : Controller.ActionStateMaps)
	{
		if (Map.iActionHash == 0 && !Map.strActionName.empty())
			Map.iActionHash = Hash_String(StringToWString(Map.strActionName).c_str());

		if (Map.iStateIndex >= 0 && Map.iStateIndex < Controller.States.size())
		{
			Map.iStateHash = Controller.States[Map.iStateIndex].iNameHash;
			continue;
		}

		Map.iStateIndex = -1;

		for (_uint i = 0; i < Controller.States.size(); i++)
		{
			if (Controller.States[i].iNameHash == Map.iStateHash)
			{
				Map.iStateIndex = static_cast<_int>(i);
				break;
			}
		}
	}

	for (ANIM_SEQUENCE& Sequence : EditData.Sequences)
	{
		for (ANIM_SEQUENCE_STEP& Step : Sequence.Steps)
		{
			for (ANIM_CONTROLLER_CONDITION& Condition : Step.HoldConditions)
			{
				Condition.iParameterIndex = -1;

				auto ParamIter = ParamIndexByHash.find(Condition.iParameterHash);
				if (ParamIter != ParamIndexByHash.end())
					Condition.iParameterIndex = ParamIter->second;
			}
		}
	}

	for (ANIM_POSE_LAYER_RULE& Rule : Controller.PoseLayerRules)
	{
		Rule.iMotionIndex = -1;

		if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
		{
			auto iter = ClipIndexByHash.find(Rule.iMotionHash);
			if (iter != ClipIndexByHash.end())
				Rule.iMotionIndex = iter->second;
		}
		else if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
		{
			auto iter = SequenceIndexByHash.find(Rule.iMotionHash);
			if (iter != SequenceIndexByHash.end())
				Rule.iMotionIndex = iter->second;
		}


		Rule.iMaskRootBoneIndex = -1;

		for (_uint iBone = 0; iBone < m_Bones.size(); ++iBone)
		{
			const ANIMATOR_BONE_DESC& Bone = m_Bones[iBone];
			const _uint64 iBoneHash = Bone.iNameHash != 0 ? Bone.iNameHash : Hash_String(StringToWString(Bone.strName).c_str());

			if (Rule.iMaskRootBoneHash == iBoneHash ||
				(!Rule.strMaskRootBoneName.empty() && Rule.strMaskRootBoneName == Bone.strName))
			{
				Rule.strMaskRootBoneName = Bone.strName;
				Rule.iMaskRootBoneHash = iBoneHash;
				Rule.iMaskRootBoneIndex = static_cast<_int>(iBone);
				break;
			}
		}

		for (ANIM_CONTROLLER_CONDITION& Condition : Rule.Conditions)
		{
			Condition.iParameterIndex = -1;

			auto ParamIter = ParamIndexByHash.find(Condition.iParameterHash);
			if (ParamIter != ParamIndexByHash.end())
				Condition.iParameterIndex = ParamIter->second;
		}
	}
}

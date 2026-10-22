#pragma once
#include "UIViewModelBase.h"
#include "DialogueData.h"

NS_BEGIN(GameCore)

enum class NPC_DIALOGUE_VIEW_MODE
{
	NONE,
	DIALOGUE,
	SERVICE_SELECTION,
	END
};

class CNPCDialogueViewModel : public CUIViewModelBase
{
private:
	CNPCDialogueViewModel() = default;
	virtual ~CNPCDialogueViewModel() = default;

public:
	// 새 NPC 대화를 시작한다.
	void						Begin_Dialogue(const _wstring& strSpeakerName, const _wstring& strDialogueText);

	// 일반 대사 화면으로 변경한다.
	void						Set_DialogueLine(const _wstring& strSpeakerName, const _wstring& strDialogueText);

	// 서비스 선택 화면으로 변경한다.
	void						Show_ServiceOptions(const vector<_wstring>& ServiceLabels);

	// 대화창을 닫을 상태로 변경한다.
	void						Close_Dialogue();

public:
    _bool Is_Visible() const { return m_bVisible; }

    NPC_DIALOGUE_VIEW_MODE		Get_ViewMode() const { return m_eViewMode; }
    const _wstring&				Get_SpeakerName() const { return m_strSpeakerName; }
	const _wstring&				Get_DialogueText() const { return m_strDialogueText; }
	const _wstring&				Get_PrimaryActionText() const { return m_strPrimaryActionText; }
	_uint						Get_ServiceCount() const { return m_iServiceCount; }
	const _wstring&				Get_ServiceLabel(_uint iIndex) const { return m_strServiceLabels[iIndex]; }

private:
	void						Clear_ServiceOptions();


private:
	_bool						m_bVisible = { false };
	NPC_DIALOGUE_VIEW_MODE		m_eViewMode = { NPC_DIALOGUE_VIEW_MODE::NONE };

	_wstring					m_strSpeakerName;
	_wstring					m_strDialogueText;
	_wstring					m_strPrimaryActionText = { L"다음" };

	_uint						m_iServiceCount = { 0 };
	_wstring					m_strServiceLabels[NPC_DIALOGUE_MAX_SERVICE_COUNT];


public:
	static CNPCDialogueViewModel* Create();
	virtual void Free() override;
};

NS_END
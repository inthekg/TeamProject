#include "InputComponent.h"
#include "GameInstance.h"
#include "Engine_Utility.h"


CInputComponent::CInputComponent()
	: CComponent(nullptr, nullptr)
{

}


HRESULT CInputComponent::Initialize_Prototype()
{
	Setup_Bindings();
	return S_OK;
}

void CInputComponent::Setup_Bindings()
{
	m_KeyMaps[DIK_LSHIFT] = ACTION_INPUTTYPE::GUARD;
	m_KeyMaps[DIK_SPACE] = ACTION_INPUTTYPE::EX_MOVE;
	m_KeyMaps[DIK_LCONTROL] = ACTION_INPUTTYPE::LEGION;

	m_KeyMaps[DIK_F] = ACTION_INPUTTYPE::FABLE_BASE;
	m_KeyMaps[DIK_V] = ACTION_INPUTTYPE::FATAL;

	m_KeyMaps[DIK_T] = ACTION_INPUTTYPE::HEAL;
	m_KeyMaps[DIK_G] = ACTION_INPUTTYPE::GRINDER;
	m_KeyMaps[DIK_R] = ACTION_INPUTTYPE::ITEM_USE;

	m_KeyMaps[DIK_TAB] = ACTION_INPUTTYPE::SWAP;
	m_KeyMaps[DIK_E] = ACTION_INPUTTYPE::INTERACT;
	m_KeyMaps[DIK_X] = ACTION_INPUTTYPE::LEGION_EX;
	m_KeyMaps[DIK_C] = ACTION_INPUTTYPE::LAMP;
	//m_KeyMaps[ETOUI(DIMM::X)] = ACTION_INPUTTYPE::LIGHT_ATTACK;
	//m_KeyMaps[ETOUI(DIMM::Y)] = ACTION_INPUTTYPE::HEAVY_ATTACK;

	
}

void CInputComponent::Update(_float fTimeDelta, _bool bInputBlocked)
{
	if (bInputBlocked)
	{
		m_iRawAxisX = 0;
		m_iRawAxisY = 0;

		Clear_ButtonRecord();
		Update_InputLogic(fTimeDelta);
		m_bPrevBlocked = true;
		return;
	}

	//입력 락이 풀린 첫 프레임
	if (m_bPrevBlocked)
	{
		m_bPrevBlocked = false;
		Ignore_HoldKey();
	}

	/* 입력 축 계산 */

	if (m_pGameInstance->Key_Down(DIK_D)) m_bLastXWasRight = true;
	if (m_pGameInstance->Key_Down(DIK_A)) m_bLastXWasRight = false;
	if (m_pGameInstance->Key_Down(DIK_W)) m_bLastYWasFront = true;
	if (m_pGameInstance->Key_Down(DIK_S)) m_bLastYWasFront = false;

	//현재 눌림 상태
	const _bool pushW = m_pGameInstance->Key_Pressing(DIK_W);
	const _bool pushS = m_pGameInstance->Key_Pressing(DIK_S);
	const _bool pushD = m_pGameInstance->Key_Pressing(DIK_D);
	const _bool pushA = m_pGameInstance->Key_Pressing(DIK_A);

	Calculate_InputKey(pushD, pushA, m_bLastXWasRight, m_iRawAxisX);
	Calculate_InputKey(pushW, pushS, m_bLastYWasFront, m_iRawAxisY);

	/* Update Button Records */
	for (const auto& key : m_KeyMaps)
	{
		ButtonRecord& rc = m_ButtonRecords[key.second];
		_bool bPressing = m_pGameInstance->Key_Pressing(key.first);

		if (rc.bIgnoreHold)
		{
			if (bPressing)
			{
				//무시함 
				rc.bPrevDown = false;
				rc.bDown = false;
				rc.fHoldTime = 0.f;
				continue;
			}
			else rc.bIgnoreHold = false;
		}

		rc.bPrevDown = rc.bDown;
		rc.bDown = bPressing;
		if (rc.bDown)
		{
			if (rc.IsJustPressed()) rc.fHoldTime = 0.f;
			rc.fHoldTime += fTimeDelta;
		}
	}

	auto Update_MouseButton = [&](ACTION_INPUTTYPE eType, DIMB eBtn)
		{
			ButtonRecord& rc = m_ButtonRecords[eType];
			_bool bPressing = m_pGameInstance->Mouse_Pressing(eBtn);
			
			if (rc.bIgnoreHold)
			{
				if (bPressing)
				{
					//무시함 
					rc.bPrevDown = false;
					rc.bDown = false;
					rc.fHoldTime = 0.f;
					return;
				}
				else rc.bIgnoreHold = false;
			}

			rc.bPrevDown = rc.bDown;
			rc.bDown = bPressing;
			if (rc.bDown)
			{
				if (rc.IsJustPressed()) rc.fHoldTime = 0.f;
				rc.fHoldTime += fTimeDelta;
			}
		};

	Update_MouseButton(ACTION_INPUTTYPE::LIGHT_ATTACK, DIMB::LBUTTON);
	Update_MouseButton(ACTION_INPUTTYPE::HEAVY_ATTACK, DIMB::RBUTTON);

	/* 위에서 갱신한 데이터로 Update Action State*/
	Update_InputLogic(fTimeDelta);
}

void CInputComponent::Update_InputLogic(_float fTimeDelta)
{
	/* Ex Move 버튼 처리(달리기,구르기..) */
	ButtonRecord& exRec = m_ButtonRecords[ACTION_INPUTTYPE::EX_MOVE];
	_bool bIsRunning = m_ActionStates[PlayerActionID::Run].bActive;

	if(m_fJumpWindowTimer > 0.f) m_fJumpWindowTimer -= fTimeDelta;
		
	if(exRec.IsJustPressed() && m_fJumpWindowTimer > 0.f)
	{
		Push_Action_BufferTime(PlayerActionID::Jump, BUFFER_TIME);
		m_fJumpWindowTimer = 0.f;
		m_bJumpedThisPress = true; // 구르기 방지 
	}
	else if (exRec.bDown && exRec.fHoldTime >= RUN_HOLD_TIME)
	{
		Set_Action_Active(PlayerActionID::Run, true);
	}
	else if(!m_bJumpedThisPress && exRec.IsJustReleased())
	{
		if (bIsRunning) m_fJumpWindowTimer = JUMP_WINDOW_TIME; // 달리기 중 점프 가능 시간
		else
		{
			Push_Action_BufferTime(PlayerActionID::Dodge, BUFFER_TIME);
		}

	}

	if(!exRec.bDown)
	{
		Set_Action_Active(PlayerActionID::Run, false);
		exRec.fHoldTime = 0.f;
		m_bJumpedThisPress = false;
	}

	/* Item */
	if (m_ButtonRecords[ACTION_INPUTTYPE::LAMP].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Lamp, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::HEAL].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Heal, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::GRINDER].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Grinder, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::ITEM_USE].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Item_Use, BUFFER_TIME);

	Set_Action_Active(PlayerActionID::Item_Use, m_ButtonRecords[ACTION_INPUTTYPE::ITEM_USE].bDown);

	/* Legion Arm */
	if (m_ButtonRecords[ACTION_INPUTTYPE::LEGION].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::LegionArm, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::LEGION_EX].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::LegionArm_Ex, BUFFER_TIME);

	/* 다른 단일 액션 */
	Set_Action_Active(PlayerActionID::Guard, m_ButtonRecords[ACTION_INPUTTYPE::GUARD].bDown);
	
	// Charger Heavy처리를위해 활성화 추가 
	Set_Action_Active(PlayerActionID::Heavy_Attack, m_ButtonRecords[ACTION_INPUTTYPE::HEAVY_ATTACK].bDown);

	if (m_ButtonRecords[ACTION_INPUTTYPE::SWAP].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::SwapWeapon, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::LIGHT_ATTACK].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Light_Attack, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::HEAVY_ATTACK].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Heavy_Attack, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::INTERACT].IsJustPressed())
		Push_Action_BufferTime(PlayerActionID::Interact, BUFFER_TIME);

	if (m_ButtonRecords[ACTION_INPUTTYPE::FABLE_BASE].IsJustPressed())
	{
		//Shift(원래 가드키) + Fatal = Fatal EX 
		if (m_ButtonRecords[ACTION_INPUTTYPE::GUARD].bDown)
		{
			Push_Action_BufferTime(PlayerActionID::Fable_EX, BUFFER_TIME);
		}
		else
			Push_Action_BufferTime(PlayerActionID::Fable_Arts, BUFFER_TIME);
	}

	if (m_ButtonRecords[ACTION_INPUTTYPE::FATAL].IsJustPressed())
	{
		Push_Action_BufferTime(PlayerActionID::Fatal, BUFFER_TIME);
	}

	for(auto& pair: m_ActionStates)
	{
		if(pair.second.fBufferTimer > 0.f)
		{
			pair.second.fBufferTimer -= fTimeDelta;
			if(pair.second.fBufferTimer <= 0.f)
			{
				pair.second.fBufferTimer = 0.f;
			}
		}
	}
}

void CInputComponent::Clear_ButtonRecord()
{
	for(auto& pair : m_ButtonRecords)
	{
		pair.second.bDown = false;
		pair.second.bPrevDown = false;
		pair.second.fHoldTime = 0.f;
		pair.second.bIgnoreHold = false;
	}
	
}


_bool CInputComponent::Peek(PlayerActionID eActionID)
{
	return m_ActionStates[eActionID].fBufferTimer > 0.f;
}

_bool CInputComponent::IsHold(PlayerActionID eActionID)
{
	return m_ActionStates[eActionID].bActive;
}

_bool CInputComponent::Consume(PlayerActionID eActionID)
{
	if(m_ActionStates[eActionID].fBufferTimer > 0.f)
	{
		m_ActionStates[eActionID].fBufferTimer = 0.f;
		return true;
	}
	return false;
}


void CInputComponent::Ignore_HoldKey()
{
	for(const auto& Key : m_KeyMaps)
	{
		if(m_pGameInstance->Key_Pressing(Key.first))
			m_ButtonRecords[Key.second].bIgnoreHold = true;
	}

	//마우스도 무시 
	if (m_pGameInstance->Mouse_Pressing(DIMB::LBUTTON))
		m_ButtonRecords[ACTION_INPUTTYPE::LIGHT_ATTACK].bIgnoreHold = true;

	if (m_pGameInstance->Mouse_Pressing(DIMB::RBUTTON))
		m_ButtonRecords[ACTION_INPUTTYPE::HEAVY_ATTACK].bIgnoreHold = true;
}

void CInputComponent::Clear()
{
	Clear_ButtonRecord();
	for(auto& pair: m_ActionStates)
	{
		pair.second.bActive = false;
		pair.second.fBufferTimer = 0.f;
	}

	m_fJumpWindowTimer = 0.f;
	m_bJumpedThisPress = false;
}

void CInputComponent::Calculate_InputKey(_bool bPushPositive, _bool bPushNegative, _bool bLastPositive, _int& iOutRaw)
{
	//반대키가 둘 다 눌렸을 때
	if (bPushPositive && bPushNegative)
	{
		//마지막에 눌린 키가 양수면 양수로, 음수면 음수로
		if (bLastPositive) iOutRaw = 1;
		else iOutRaw = -1;
	}
	//양수키만 눌렸을 때
	else if (bPushPositive) iOutRaw = 1;
	//음수키만 눌렸을 때
	else if (bPushNegative) iOutRaw = -1;
	else iOutRaw = 0;
}

void CInputComponent::Set_Action_Active(PlayerActionID eActionID, _bool bActive)
{
	m_ActionStates[eActionID].bActive = bActive;
}

void CInputComponent::Push_Action_BufferTime(PlayerActionID eActionID, _float fBufferTime)
{
	m_ActionStates[eActionID].fBufferTimer = fBufferTime;
}

CInputComponent* CInputComponent::Create()
{
	CInputComponent* pInstance = new CInputComponent();
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CInputComponent");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CInputComponent::Free()
{
	__super::Free();
}

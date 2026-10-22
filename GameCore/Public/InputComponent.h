#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)
class GAMECORE_DLL CInputComponent : public CComponent
{
	typedef struct tagButtonRecord
	{
		_bool	bDown = { false };
		_bool	bPrevDown = { false };
		_float	fHoldTime = {0.f};
		//inputlock순간에 눌렸으면 뗄 때 까지 무시하는 처리용
		_bool	bIgnoreHold = { false };
		_bool	IsJustPressed() const { return bDown && !bPrevDown; }
		_bool	IsJustReleased() const { return !bDown && bPrevDown; }
	} ButtonRecord;

	typedef struct tagInputState
	{
		_bool bActive = { false };
		_float fBufferTimer = { 0.f };
	} InputState;

private:
	CInputComponent();
	virtual ~CInputComponent() = default;

public:
	_int											Get_RawAxisX() const { return m_iRawAxisX; }
	_int											Get_RawAxisY() const { return m_iRawAxisY; }
	_bool 											Has_MoveInput() const { return (m_iRawAxisX != 0 || m_iRawAxisY != 0); }
public:
	virtual HRESULT									Initialize_Prototype() override;

	//키 입력 업데이트 
	void											Update(_float fTimeDelta, _bool bInputBlocked);
	//활성화인지 체크만
	_bool											Peek(PlayerActionID eActionID);
	//눌렸냐 (가드용)
	_bool											IsHold(PlayerActionID eActionID);

	//액션 State 소비
	_bool											Consume(PlayerActionID eActionID);
	virtual void									Clear() override;
private:
	void											Update_InputLogic(_float fTimeDelta);
	void											Clear_ButtonRecord();

private:
	void											Setup_Bindings();
	void											Ignore_HoldKey();
	void 											Calculate_InputKey(_bool bPushPositive, _bool bPushNegative, _bool bLastPositive, _int& iOutRaw);
	//액션 활성화
	void											Set_Action_Active(PlayerActionID eActionID, _bool bActive);
	//액션 특정 시간동안 활성화
	void											Push_Action_BufferTime(PlayerActionID eActionID, _float fBufferTime);
private:
	static constexpr _float 						RUN_HOLD_TIME			= { 0.5f };
	static constexpr _float							BUFFER_TIME				= { 0.2f };
	static constexpr _float							JUMP_WINDOW_TIME		= { 0.15f };
	//순수 입력 값 
	_int											m_iRawAxisX				= {};
	_int											m_iRawAxisY				= {};

	//마지막에 눌린 키 찾는 용도
	_bool											m_bLastXWasRight		= {};
	_bool											m_bLastYWasFront		= {};

	_float											m_fJumpWindowTimer		= { 0.f };
	_bool											m_bJumpedThisPress		= { false };
	
	//UI 종료때 KEY holding 무시용
	_bool											m_bPrevBlocked			= { false };

	//키맵핑 데이터 
	unordered_map<_ubyte, ACTION_INPUTTYPE>			m_KeyMaps;
	//내부 계산용 
	unordered_map<ACTION_INPUTTYPE, ButtonRecord>	m_ButtonRecords;
	//실제 데이터 
	unordered_map<PlayerActionID, InputState>		m_ActionStates;
public:
	static CInputComponent* Create();
	virtual void Free() override;
};

NS_END
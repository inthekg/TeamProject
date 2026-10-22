#ifndef InputDev_h__
#define InputDev_h__

#include "Base.h"

NS_BEGIN(Engine)

class  CInput_Device final : public CBase
{
private:
	CInput_Device(void);
	virtual ~CInput_Device(void) = default;
	
public:
	_byte	Get_DIKeyState(_ubyte byKeyID)		
	{ 
		return m_byKeyState[byKeyID]; 
	}

	_byte	Get_DIMouseState(DIMB eMouse)
	{ 	
		return m_tMouseState.rgbButtons[static_cast<_uint>(eMouse)]; 	
	}

	// 현재 마우스의 특정 축 좌표를 반환
	_long	Get_DIMouseMove(DIMM eMouseState)
	{	
		return *((reinterpret_cast<_long*>(&m_tMouseState)) + ETOUI(eMouseState));
	}
	
	// 현재 마우스 위치를 게임 창의 좌측 상단 기준 좌표로 반환
	_bool	Get_MouseClientPosition(_float2* pOutPosition) const;

public:
	HRESULT					Initialize(HINSTANCE hInst, HWND hWnd);
	void					Update(_float fTimeDelta);

public:
	bool					Key_Down(_ubyte byKeyID);
	bool					Key_Pressing(_ubyte byKeyID);
	bool					Key_Up(_ubyte byKeyID);

	bool					Mouse_Down(DIMB eMouse);
	bool					Mouse_Pressing(DIMB eMouse);
	bool					Mouse_Up(DIMB eMouse);

public:
	/* 커서 고정 */
	void					Set_CursorState(CURSOR_STATE eState);
	CURSOR_STATE			Get_CursorState() const { return m_eCursorState; }
	
	void					Notify_WindowInActive();

	void					Update_CursorLock();
	void					Release_CursorLock();
	void					Show_Cursor(_bool bShow);
private:
	LPDIRECTINPUT8			m_pInputSDK				= { nullptr };

private:
	LPDIRECTINPUTDEVICE8	m_pKeyBoard				= { nullptr };
	LPDIRECTINPUTDEVICE8	m_pMouse				= { nullptr };

private:
	HWND					m_hWnd					= { nullptr }; // UI용 마우스 절대 위치

private:
	
	_byte					m_byKeyState[256]		= {};	// 키보드에 있는 모든 키값을 저장하기 위한 변수
	_byte					m_byPrevKeyState[256]	= {};	// 이전 프레임의 키보드 상태값 

	DIMOUSESTATE			m_tMouseState			= {};	// 마우스 상태값
	DIMOUSESTATE			m_tPrevMouseState		= {};	// 이전 프레임의 마우스 상태값

private:
	CURSOR_STATE			m_eCursorState			= { CURSOR_STATE::FREE };
	_bool					m_bCursorVisible		= { true };
	_bool					m_bLockApplied			= { false };

public:
	static CInput_Device*	Create(HINSTANCE hInstance, HWND hWnd);
	virtual void			Free(void);

};
NS_END

#endif // InputDev_h__



#include "Input_Device.h"
#include "GameInstance.h"

CInput_Device::CInput_Device()
{

}

// 추가
_bool CInput_Device::Get_MouseClientPosition(_float2* pOutPosition) const
{
	if (pOutPosition == nullptr || m_hWnd == nullptr)
		return false;

	// 게임 창이 활성화된 동안에만 UI 마우스 입력을 허용한다.
	if (GetForegroundWindow() != m_hWnd)
		return false;

	POINT Point = {};

	// 모니터 전체 기준 마우스 좌표를 얻는다.
	if (!GetCursorPos(&Point))
		return false;

	// 모니터 좌표를 게임 창 내부 좌표로 변환한다.
	if (!ScreenToClient(m_hWnd, &Point))
		return false;

	pOutPosition->x = static_cast<_float>(Point.x);
	pOutPosition->y = static_cast<_float>(Point.y);

	return true;
}

HRESULT CInput_Device::Initialize(HINSTANCE hInst, HWND hWnd)
{
	// 추가
	if (hWnd == nullptr)
		return E_FAIL;

	m_hWnd = hWnd;
	//

	if (FAILED(DirectInput8Create(hInst,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&m_pInputSDK,
		NULL)))
		return E_FAIL;

	if (FAILED(m_pInputSDK->CreateDevice(GUID_SysKeyboard, &m_pKeyBoard, nullptr)))
		return E_FAIL;

	m_pKeyBoard->SetDataFormat(&c_dfDIKeyboard);
	m_pKeyBoard->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	m_pKeyBoard->Acquire();


	if (FAILED(m_pInputSDK->CreateDevice(GUID_SysMouse, &m_pMouse, nullptr)))
		return E_FAIL;

	m_pMouse->SetDataFormat(&c_dfDIMouse);

	m_pMouse->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	m_pMouse->Acquire();

	return S_OK;
}

void CInput_Device::Update(_float fTimeDelta)
{
	Update_CursorLock();

	memcpy(m_byPrevKeyState, m_byKeyState, sizeof(m_byPrevKeyState));
	memcpy(&m_tPrevMouseState, &m_tMouseState, sizeof(DIMOUSESTATE));

	m_pKeyBoard->GetDeviceState(256, m_byKeyState);
	m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &m_tMouseState);

}

bool CInput_Device::Key_Down(_ubyte byKeyID)
{
	bool bResult = (!(m_byPrevKeyState[byKeyID] & 0x80) && (m_byKeyState[byKeyID] & 0x80));
	return bResult;
}

bool CInput_Device::Key_Pressing(_ubyte byKeyID)
{
	return (m_byKeyState[byKeyID] & 0x80);
}

bool CInput_Device::Key_Up(_ubyte byKeyID)
{
	bool bResult = ((m_byPrevKeyState[byKeyID] & 0x80) && !(m_byKeyState[byKeyID] & 0x80));
	return bResult;
}

bool CInput_Device::Mouse_Down(DIMB eMouse)
{
	_uint iMouseState = ETOUI(eMouse);
	bool bResult = !(m_tPrevMouseState.rgbButtons[iMouseState] & 0x80) && (m_tMouseState.rgbButtons[iMouseState] & 0x80);
	return bResult;
}

bool CInput_Device::Mouse_Pressing(DIMB eMouse)
{
	return (m_tMouseState.rgbButtons[ETOUI(eMouse)] & 0x80);
}

bool CInput_Device::Mouse_Up(DIMB eMouse)
{
	_uint iMouseState = ETOUI(eMouse);
	bool bResult = (m_tPrevMouseState.rgbButtons[iMouseState] & 0x80) && !(m_tMouseState.rgbButtons[iMouseState] & 0x80);
	return bResult;
}

void CInput_Device::Set_CursorState(CURSOR_STATE eState)
{
	if(m_eCursorState == eState) 
		return;

	m_eCursorState = eState;

	// 마우스 표시 여부 추가
	if(CURSOR_STATE::FREE == eState)
		Release_CursorLock();
}


void CInput_Device::Notify_WindowInActive()
{
	Release_CursorLock();
}

void CInput_Device::Update_CursorLock()
{
	if (m_hWnd == nullptr) return;

	//FREE거나 작업창이 아니면 해제
	if (CURSOR_STATE::FREE == m_eCursorState || GetForegroundWindow() != m_hWnd)
	{
		Release_CursorLock();
		return;
	}

	RECT rcClient = {};
	if (!GetClientRect(m_hWnd, &rcClient))
		return;

	//최소화되면 크기가 0임
	if (rcClient.right <= rcClient.left || rcClient.bottom <= rcClient.top)
		return;

	POINT ptLeftTop = { rcClient.left,  rcClient.top };
	POINT ptRightBottom = { rcClient.right, rcClient.bottom };

	if (!ClientToScreen(m_hWnd, &ptLeftTop) || !ClientToScreen(m_hWnd, &ptRightBottom))
		return;

	RECT rcClip = { ptLeftTop.x , ptLeftTop.y, ptRightBottom.x, ptRightBottom.y };

	ClipCursor(&rcClip);
	//커서 이미지 생기면 처리 
	Show_Cursor(CURSOR_STATE::CLIP == m_eCursorState);

	if (m_eCursorState == CURSOR_STATE::CENTER)
	{
		_int iCenterX = (rcClip.left + rcClip.right) >> 1;
		_int iCenterY = (rcClip.top + rcClip.bottom) >> 1;

		POINT ptCursor = {};

		//중앙에 고정 
		if (!GetCursorPos(&ptCursor) || ptCursor.x != iCenterX || ptCursor.y != iCenterY)
			SetCursorPos(iCenterX, iCenterY);
	}

	m_bLockApplied = true;
}

void CInput_Device::Release_CursorLock()
{
	if (!m_bLockApplied) return;

	ClipCursor(nullptr);
	Show_Cursor(true);

	m_bLockApplied = false;
}

void CInput_Device::Show_Cursor(_bool bShow)
{
	if (m_bCursorVisible == bShow)
		return;

	m_bCursorVisible = bShow;

	if (bShow)
	{
		//ShowCursor 내부 카운트가 조건을 만족할때까지만 
		while (ShowCursor(TRUE) < 0);
	}
	else
	{
		while (ShowCursor(FALSE) >= 0);
	}
}

CInput_Device* CInput_Device::Create(HINSTANCE hInstance, HWND hWnd)
{
	CInput_Device* pInstance = new CInput_Device();

	if (FAILED(pInstance->Initialize(hInstance, hWnd)))
	{
		MSG_BOX("Failed to Created : CInput_Device");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CInput_Device::Free()
{
	__super::Free();

	ClipCursor(nullptr);
	Show_Cursor(true);

	m_hWnd = nullptr;

	Safe_Release(m_pKeyBoard);
	Safe_Release(m_pMouse);
	Safe_Release(m_pInputSDK);
}


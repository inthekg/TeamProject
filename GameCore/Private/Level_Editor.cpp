#include "Level_Editor.h"
#include "Camera_Free.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameInstance.h"

CLevel_Editor::CLevel_Editor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
    : CLevel{ pDevice, pContext, hWnd }
{
}

HRESULT CLevel_Editor::Initialize()
{
     __super::Initialize();

   // if (FAILED(Ready_Lights()))
   //     return E_FAIL;
   //
   //
      if (FAILED(Rendy_Layer_Sky(TEXT("Layer_Sky"))))
        return E_FAIL;
   
   
    if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
        return E_FAIL;
     
    return S_OK; 
}

void CLevel_Editor::Update(_float fTimeDelta)
{
    if (m_bIsFirst)
    {
        m_pGameInstance->Broadcast(EVENT_TYPE::START_LEVEL, nullptr);
        m_bIsFirst = false;
    }
}

HRESULT CLevel_Editor::Render()
{
#ifdef _DEBUG
    SetWindowText(m_hWnd, TEXT("에디터레벨입니다."));
#endif
    return S_OK;
}

//HRESULT CLevel_Editor::Ready_Lights()
//{
//    LIGHT_DESC      LightDesc{};
//
//    LightDesc.eType = LIGHT::DIRECTIONAL;
//    LightDesc.vDiffuse = _float4(0.4f, 0.4f, 0.4f, 0.4f);
//    LightDesc.vAmbient = _float4(0.2f, 0.2f, 0.2f, 0.2f);
//    LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);
//    LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);
//
//    if (FAILED(m_pGameInstance->Add_Light(LIGHT::DIRECTIONAL, LightDesc)))
//        return E_FAIL;
//
//    return S_OK;
//}
//
HRESULT CLevel_Editor::Ready_Layer_Camera(const _wstring& strLayerTag)
{
    CCamera_Free::CAMERA_FREE_DESC      CameraDesc{};

    CameraDesc.vEye = _float3(0.f, 3.f, -6.f);
    CameraDesc.vAt = _float3(0.f, 0.f, 0.f);
    CameraDesc.fFovy = XMConvertToRadians(60.f);
    CameraDesc.fNear = 0.1f;
    CameraDesc.fFar = 500.f;
    CameraDesc.fSpeedPerSec = 20.f;
    CameraDesc.fRotationPerSec = XMConvertToRadians(180.f);
    CameraDesc.fMouseSensor = 0.02f;


    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_CameraFree"),
        ETOUI(LEVEL::GAMEPLAY), strLayerTag, &CameraDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Editor::Rendy_Layer_Sky(const _wstring& strLayerTag)
{
    if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Sky"), ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Sky"))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bake_IBL()))
        return E_FAIL;


    return S_OK;
}

CLevel_Editor* CLevel_Editor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd)
{
    CLevel_Editor* pInstance = new CLevel_Editor(pDevice, pContext, hWnd);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CLevel_Editor");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLevel_Editor::Free()
{
    __super::Free();
}

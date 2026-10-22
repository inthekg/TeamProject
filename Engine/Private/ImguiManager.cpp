#include "ImguiManager.h"

CImguiManager::CImguiManager()
{
}

HRESULT CImguiManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hWnd)
{
    m_pDevice = device;
    m_pContext = deviceContext;

    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 13.0f;
    io.Fonts->AddFontDefault(&fontConfig);

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pContext);

    return S_OK;
}

void CImguiManager::Begin_Frame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    //ImGui::DockSpaceOverViewport();
}

void CImguiManager::Render()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

CImguiManager* CImguiManager::Create(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hWnd)
{
    CImguiManager* pInstance = new CImguiManager();

    if (FAILED(pInstance->Initialize(device, deviceContext, hWnd)))
    {
        MSG_BOX("Failed to Created : CImguiManager");
        Safe_Release(pInstance);
    }

    return pInstance;
}
void CImguiManager::Free()
{
    __super::Free();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}

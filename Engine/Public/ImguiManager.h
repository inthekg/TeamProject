#pragma once
#include "Base.h"
#include "Imgui_Header.h"


NS_BEGIN(Engine)
class CImguiManager : public CBase
{
public:
    CImguiManager();
    virtual ~CImguiManager() = default;

public:
    HRESULT                 Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hWnd);
    void                    Begin_Frame();
    void                    Render();

private:
    ID3D11Device*           m_pDevice = { nullptr };
    ID3D11DeviceContext*    m_pContext = { nullptr };

public:
    static CImguiManager*   Create(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hWnd);
    virtual void            Free() override;

};

NS_END
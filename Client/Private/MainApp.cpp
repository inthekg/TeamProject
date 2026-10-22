#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "AIObject.h"
#include "StaticRigidBody.h"
#include "Trigger.h"
#include "Level_Logo.h"

CMainApp::CMainApp()
    : m_pGameInstance { CGameInstance::Get_Instance() }
{
}

HRESULT CMainApp::Initialize()
{
    // 수정
    srand((unsigned)time(nullptr));

    ENGINE_DESC         EngineDesc{};
    EngineDesc.hInstance = g_hInstance;
    EngineDesc.hWnd = g_hWnd;
    EngineDesc.eWinMode = WINMODE::WIN;
    EngineDesc.iViewportWidth = g_iWinSizeX;
    EngineDesc.iViewportHeight = g_iWinSizeY;
    EngineDesc.iNumLevels = ETOUI(LEVEL::END);

    if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
    {
        MSG_BOX("Failed to Initialize : Engine");
        return E_FAIL;
    }
        
    //RESOURCE_DIR : Client 전처리기 매크로 속성에서 추가해놓은 리소스 경로 
    m_pGameInstance->Set_ResourcePath(TEXT(RESOURCE_DIR));
    //반드시 리소스 경로 지정후 Sound 세팅 
    m_pGameInstance->Ready_Sound();

#ifdef _DEBUG
    //Client의 Engine의 Imgui Context로 Set
    ImGui::SetCurrentContext(m_pGameInstance->Get_ImguiContext());

    //Engine Imgui Memory 할당기로 Set
    ImGuiMemAllocFunc alloc;
    ImGuiMemFreeFunc freeFn;
    void* userData;
    ImGui::GetAllocatorFunctions(&alloc, &freeFn, &userData);
    ImGui::SetAllocatorFunctions(alloc, freeFn, userData);
#endif

    //if (FAILED(Ready_Fonts()))
    //    return E_FAIL;

    if (FAILED(Ready_Prototype_For_Static()))
        return E_FAIL;

    if (FAILED(Start_Level(LEVEL::GAMEPLAY)))
        return E_FAIL;

    m_pGameInstance->Set_CursorState(CURSOR_STATE::CLIP);

    return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
    m_pGameInstance->Update_Engine(fTimeDelta);
}


HRESULT CMainApp::Render()
{
    if (FAILED(m_pGameInstance->Begin_Draw()))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Draw()))
        return E_FAIL;



    if (FAILED(m_pGameInstance->End_Draw()))
        return E_FAIL;

    return S_OK;
}

HRESULT CMainApp::Ready_Prototype_For_Static()
{
    /* VIBuffer ---------------------------------------------------------------------------*/
    // VIBuffer는 VIBuffer..::Initialize에서  Set_Name(..) 으로 설정한 이름으로 등록됨.

    /* Register_VIBuffer_Rect */
    if (FAILED(m_pGameInstance->Register_VIBuffer(CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Register_VIBuffer_Cube */
    if (FAILED(m_pGameInstance->Register_VIBuffer(CVIBuffer_Cube::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Register_VIBuffer_PXDebugLine */
    if (FAILED(m_pGameInstance->Register_VIBuffer(CVIBuffer_PXDebugLine::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    /* Shader ---------------------------------------------------------------------------*/

    /* Register_Shader_VtxTex*/    
    {
        CShader::SHADER_DESC VtxTex_ShaderDesc;
        VtxTex_ShaderDesc.strShaderName = L"Shader_VtxTex";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        VtxTex_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_VtxTex.hlsl";
        //실행할 셰이더 함수 이름. String
        VtxTex_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        VtxTex_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, VtxTex_ShaderDesc))))
            return E_FAIL;
    }
   
    /* Register_Shader_VtxMesh */    
    {
        CShader::SHADER_DESC VtxMesh_ShaderDesc;
        VtxMesh_ShaderDesc.strShaderName = L"Shader_VtxMesh";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        VtxMesh_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_VtxMesh.hlsl";
        //실행할 셰이더 함수 이름. String
        VtxMesh_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        VtxMesh_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, VtxMesh_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_VtxAnimMesh*/
    {
        CShader::SHADER_DESC VtxAnimMesh_ShaderDesc;
        VtxAnimMesh_ShaderDesc.strShaderName = L"Shader_VtxAnimMesh";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        VtxAnimMesh_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_VtxAnimMesh.hlsl";
        //실행할 셰이더 함수 이름. String
        VtxAnimMesh_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        VtxAnimMesh_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, VtxAnimMesh_ShaderDesc))))
            return E_FAIL;

    }

    /* Register_Shader_MapInstance*/
    {
        CShader::SHADER_DESC MapInstance_ShaderDesc;
        MapInstance_ShaderDesc.strShaderName = L"Shader_MapInstance";
        MapInstance_ShaderDesc.strVShaderFilePath =
            wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_MapInstance.hlsl";
        MapInstance_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        MapInstance_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        MapInstance_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INDEXED_INSTANCE_DESC::Elements);
        MapInstance_ShaderDesc.iNumElements = VTXMAP_INDEXED_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, MapInstance_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Line*/
    {
        CShader::SHADER_DESC Line_ShaderDesc;
        Line_ShaderDesc.strShaderName = L"Shader_Line";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        Line_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Line.hlsl";
        //실행할 셰이더 함수 이름. String
        Line_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Line_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Line_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_LineWorld*/
    {
        CShader::SHADER_DESC LineWorld_ShaderDesc;
        LineWorld_ShaderDesc.strShaderName = L"Shader_LineWorld";
        LineWorld_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_LineWorld.hlsl";
        LineWorld_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        LineWorld_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, LineWorld_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Sky*/
    {
        CShader::SHADER_DESC SkyShaderDesc{};
        SkyShaderDesc.strShaderName = TEXT("Shader_Sky");
        SkyShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + TEXT("GameCore/Bin/ShaderFiles/Shader_Sky.hlsl");
        SkyShaderDesc.strVShaderFuncName = "VS_MAIN";
        SkyShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, SkyShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Hair*/
    {
        CShader::SHADER_DESC Hair_ShaderDesc;
        Hair_ShaderDesc.strShaderName = L"Shader_Hair";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        Hair_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Hair.hlsl";
        //실행할 셰이더 함수 이름. String
        Hair_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Hair_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Hair_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_HairStatic */
    {
        // 맵에 직접 배치된 정적 Hair 메시를 등록한다.
        CShader::SHADER_DESC HairStatic_ShaderDesc;
        HairStatic_ShaderDesc.strShaderName = L"Shader_HairStatic";
        HairStatic_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Hair.hlsl";
        HairStatic_ShaderDesc.strVShaderFuncName = "VS_STATIC";
        HairStatic_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, HairStatic_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_HairInstance */
    {
        // WORLD0~3 입력을 사용하는 맵 인스턴스 Hair 메시를 등록한다.
        CShader::SHADER_DESC HairInstance_ShaderDesc;
        HairInstance_ShaderDesc.strShaderName = L"Shader_HairInstance";
        HairInstance_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Hair.hlsl";
        HairInstance_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        HairInstance_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        HairInstance_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        HairInstance_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, HairInstance_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Glass */
    {
        CShader::SHADER_DESC Glass_ShaderDesc;
        Glass_ShaderDesc.strShaderName = L"Shader_Glass";

        Glass_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Glass.hlsl";
        Glass_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Glass_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Glass_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_GlassInstance */
    {
        CShader::SHADER_DESC GlassInst_ShaderDesc;
        GlassInst_ShaderDesc.strShaderName = L"Shader_GlassInstance";
        GlassInst_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Glass.hlsl";
        GlassInst_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        GlassInst_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        GlassInst_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        GlassInst_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, GlassInst_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Eye */
    {
        CShader::SHADER_DESC Glass_ShaderDesc;
        Glass_ShaderDesc.strShaderName = L"Shader_Eye";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        Glass_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eye.hlsl";
        //실행할 셰이더 함수 이름. String
        Glass_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Glass_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Glass_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_EyeStatic */
    {
        // 맵의 Static Eye 메시만 Eye 전용 셰이더로 라우팅한다.
        CShader::SHADER_DESC EyeStatic_ShaderDesc;
        EyeStatic_ShaderDesc.strShaderName = L"Shader_EyeStatic";
        EyeStatic_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eye.hlsl";
        EyeStatic_ShaderDesc.strVShaderFuncName = "VS_STATIC";
        EyeStatic_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, EyeStatic_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_EyeInstance */
    {
        // 맵의 Instanced Eye 메시만 Eye 전용 인스턴스 진입점으로 라우팅한다.
        CShader::SHADER_DESC EyeInstance_ShaderDesc;
        EyeInstance_ShaderDesc.strShaderName = L"Shader_EyeInstance";
        EyeInstance_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eye.hlsl";
        EyeInstance_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        EyeInstance_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        EyeInstance_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        EyeInstance_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, EyeInstance_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Eyelash */
    {
        CShader::SHADER_DESC Eyelash_ShaderDesc;
        Eyelash_ShaderDesc.strShaderName = L"Shader_Eyelash";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        Eyelash_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eyelash.hlsl";
        //실행할 셰이더 함수 이름. String
        Eyelash_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Eyelash_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Eyelash_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_EyelashStatic */
    {
        // 맵에 직접 배치된 정적 속눈썹 메시를 등록한다.
        CShader::SHADER_DESC EyelashStatic_ShaderDesc;
        EyelashStatic_ShaderDesc.strShaderName = L"Shader_EyelashStatic";
        EyelashStatic_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eyelash.hlsl";
        EyelashStatic_ShaderDesc.strVShaderFuncName = "VS_STATIC";
        EyelashStatic_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, EyelashStatic_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_EyelashInstance */
    {
        // WORLD0~3 입력을 사용하는 맵 인스턴스 속눈썹 메시를 등록한다.
        CShader::SHADER_DESC EyelashInstance_ShaderDesc;
        EyelashInstance_ShaderDesc.strShaderName = L"Shader_EyelashInstance";
        EyelashInstance_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Eyelash.hlsl";
        EyelashInstance_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        EyelashInstance_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        EyelashInstance_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        EyelashInstance_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, EyelashInstance_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Skin */
    {
        CShader::SHADER_DESC Skin_ShaderDesc;
        Skin_ShaderDesc.strShaderName = L"Shader_Skin";
        //셰이더 주소 . VS,PS가 하나의 파일안에 있다면 VSpath에만 넣으면됨
        Skin_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Skin.hlsl";
        //실행할 셰이더 함수 이름. String
        Skin_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Skin_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Skin_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_SkinStatic */
    {
        // 맵에 직접 배치된 정적 Skin 메시를 등록한다.
        CShader::SHADER_DESC SkinStatic_ShaderDesc;
        SkinStatic_ShaderDesc.strShaderName = L"Shader_SkinStatic";
        SkinStatic_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Skin.hlsl";
        SkinStatic_ShaderDesc.strVShaderFuncName = "VS_STATIC";
        SkinStatic_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, SkinStatic_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_SkinInstance */
    {
        // WORLD0~3 입력을 사용하는 맵 인스턴스 Skin 메시를 등록한다.
        CShader::SHADER_DESC SkinInstance_ShaderDesc;
        SkinInstance_ShaderDesc.strShaderName = L"Shader_SkinInstance";
        SkinInstance_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Skin.hlsl";
        SkinInstance_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        SkinInstance_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        SkinInstance_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        SkinInstance_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, SkinInstance_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Unlit */
    {
        CShader::SHADER_DESC Unlit_ShaderDesc;
        Unlit_ShaderDesc.strShaderName = L"Shader_Unlit";
        Unlit_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Unlit.hlsl";
        Unlit_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Unlit_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Unlit_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_UnlitInstance */
    {
        CShader::SHADER_DESC UnlitInst_ShaderDesc;
        UnlitInst_ShaderDesc.strShaderName = L"Shader_UnlitInstance";
        UnlitInst_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Unlit.hlsl";
        UnlitInst_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        UnlitInst_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        UnlitInst_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        UnlitInst_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, UnlitInst_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_UnlitAnim */
    {
        CShader::SHADER_DESC UnlitAnim_ShaderDesc;
        UnlitAnim_ShaderDesc.strShaderName = L"Shader_UnlitAnim";
        UnlitAnim_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Unlit.hlsl";
        UnlitAnim_ShaderDesc.strVShaderFuncName = "VS_ANIM";
        UnlitAnim_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, UnlitAnim_ShaderDesc));
    }

    /* Register_Shader_HairFur */
    {
        CShader::SHADER_DESC HairFur_ShaderDesc;
        HairFur_ShaderDesc.strShaderName = L"Shader_HairFur";
        HairFur_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_HairFur.hlsl";
        HairFur_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        HairFur_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, HairFur_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_Water */
    {
        CShader::SHADER_DESC Water_ShaderDesc;
        Water_ShaderDesc.strShaderName = L"Shader_Water";

        Water_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Water.hlsl";
        Water_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Water_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Water_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_WaterInstance */
    {
        CShader::SHADER_DESC WaterInst_ShaderDesc;
        WaterInst_ShaderDesc.strShaderName = L"Shader_WaterInstance";
        WaterInst_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_Water.hlsl";
        WaterInst_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        WaterInst_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        WaterInst_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        WaterInst_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, WaterInst_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_ReflectionOpaque */
    {
        CShader::SHADER_DESC Reflect_ShaderDesc;
        Reflect_ShaderDesc.strShaderName = L"Shader_ReflectionOpaque";

        Reflect_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_ReflectionOpaque.hlsl";
        Reflect_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        Reflect_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, Reflect_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_ReflectionOpaqueInstance */
    {
        CShader::SHADER_DESC ReflectInst_ShaderDesc;
        ReflectInst_ShaderDesc.strShaderName = L"Shader_ReflectionOpaqueInstance";
        ReflectInst_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_ReflectionOpaque.hlsl";
        ReflectInst_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        ReflectInst_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        ReflectInst_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        ReflectInst_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, ReflectInst_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_SpiderWeb */
    {
        CShader::SHADER_DESC SpiderWeb_ShaderDesc;
        SpiderWeb_ShaderDesc.strShaderName = L"Shader_SpiderWeb";

        SpiderWeb_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SpiderWeb.hlsl";
        SpiderWeb_ShaderDesc.strVShaderFuncName = "VS_MAIN";
        SpiderWeb_ShaderDesc.strPShaderFuncName = "PS_MAIN";

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, SpiderWeb_ShaderDesc))))
            return E_FAIL;
    }

    /* Register_Shader_SpiderWebInstance */
    {
        CShader::SHADER_DESC SpiderWebInst_ShaderDesc;
        SpiderWebInst_ShaderDesc.strShaderName = L"Shader_SpiderWebInstance";
        SpiderWebInst_ShaderDesc.strVShaderFilePath = wstring(SOLUTION_DIR) + L"GameCore/Bin/ShaderFiles/Shader_SpiderWeb.hlsl";
        SpiderWebInst_ShaderDesc.strVShaderFuncName = "VS_INSTANCE";
        SpiderWebInst_ShaderDesc.strPShaderFuncName = "PS_MAIN";
        SpiderWebInst_ShaderDesc.pElements = const_cast<D3D11_INPUT_ELEMENT_DESC*>(VTXMAP_INSTANCE_DESC::Elements);
        SpiderWebInst_ShaderDesc.iNumElements = VTXMAP_INSTANCE_DESC::iNumElements;

        if (FAILED(m_pGameInstance->Register_Shader(CShader::Create(m_pDevice, m_pContext, SpiderWebInst_ShaderDesc))))
            return E_FAIL;
    }



    /* Camera Manager ---------------------------------------------------------------------------*/

    string strResourcePath = RESOURCE_DIR;
	string strCameraShakePath   = strResourcePath + "\\Camera\\Shake";
    string strCameraScenePath   = strResourcePath + "\\Camera\\Scene";
    string strCameraWorkPath    = strResourcePath + "\\Camera\\Work";
    m_pGameInstance->Load_CameraShakeDatas(strCameraShakePath);
    m_pGameInstance->Load_CameraSceneDatas(strCameraScenePath);
	m_pGameInstance->Load_CameraWorkDatas(strCameraWorkPath);
    

    /* Physic ---------------------------------------------------------------------------*/

    /*Prototype_Component_StaticRigidBody*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_StaticRigidBody"), CStaticRigidBody::Create(m_pDevice, m_pContext))))
        return E_FAIL;
    
    /*Prototype_GameObject_Trigger*/
    if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_Trigger"), CTrigger::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    return S_OK;
}


HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
    // 로고 씬부터 시작 
    CLevel* pLogoLevel = CLevel_Logo::Create(m_pDevice, m_pContext, eStartLevelID, g_hWnd);
    if (nullptr == pLogoLevel)
        return E_FAIL;

    if (FAILED(m_pGameInstance->Request_ChangeLevel(ETOI(LEVEL::LOGO), pLogoLevel)))
    {
      MSG_BOX("Failed to Change Level : CLevel_Logo");
        return E_FAIL;
    }

    //게임 플레이부터 시작 
    //CLevel* pPreLevel = CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID, g_hWnd);
    //if (nullptr == pPreLevel)
    //    return E_FAIL;
    //
    //if (FAILED(m_pGameInstance->Request_ChangeLevel(ETOI(LEVEL::LOADING), pPreLevel)))
    //    return E_FAIL;

    return S_OK;
}

CMainApp* CMainApp::Create()
{
    CMainApp* pInstance = new CMainApp();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMainApp");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMainApp::Free()
{
    __super::Free();
    CAIObject::Release_Registry();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);

    m_pGameInstance->Release_Engine();
}

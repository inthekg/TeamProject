#pragma once
#include "Base.h"
#include "UIAssetManager.h"
#include "UI_RuntimeData.h"
#include "UIFontRegistry.h"

NS_BEGIN(Engine)

class CMaterial;
class CUIObject;
class CUIButton;
class CUIControllerBase;
class CUIElement;
class CGameInstance;

class CUI_Manager final : public CBase
{
private:
    // 확정된 UI Input Owner Stack에 저장되는 원소
    struct UI_INPUT_OWNER_ENTRY
    {
        UI_HANDLE hUI = {};                                         // 입력을 소유한 런타임 UI
        UI_INPUT_OWNER_KIND eKind = UI_INPUT_OWNER_KIND::SCREEN;    // Screen, Fullscreen Modal, Overlay Modal 표시 방식
    };

    // 프레임 끝에 Input Owner Stack에 반영할 명령 종류
    enum class UI_INPUT_OWNER_COMMAND_TYPE
    {
        PUSH,       // 새로운 UI를 최상단 입력 소유자로 추가
        POP,        // 현재 최상단 UI의 입력 소유권 제거
        REMOVE      // UI Release 시 Stack 위치와 관계없이 강제 제거
    };

    // Late_Update에서 처리할 지연 Input Owner 명령
    struct UI_INPUT_OWNER_COMMAND
    {
        UI_HANDLE                   hUI = {};                                   // 명령 대상 UI
        UI_INPUT_OWNER_COMMAND_TYPE eType = UI_INPUT_OWNER_COMMAND_TYPE::PUSH;  // 수행할 명령
        UI_INPUT_OWNER_KIND         eKind = UI_INPUT_OWNER_KIND::SCREEN;        // PUSH일 때 추가할 Owner 종류
    };

private:
    CUI_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUI_Manager() = default;

public:
    HRESULT                                         Initialize();
    void                                            Priority_Update(_float fTimeDelta);
    void                                            Update(_float fTimeDelta);
    void                                            Late_Update(_float fTimeDelta);

    void                                            Submit_RenderCommand();

public:
    HRESULT                                         Add_UIObject(const _wstring& strUITag, CUIObject* pUIObject);
    HRESULT                                         Add_UIController(const _wstring& strControllerTag, CUIControllerBase* pController);
    HRESULT                                         Spawn_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);

    CUIObject*                                      Find_UIObject(const UI_HANDLE& Handle);
    CUIObject*                                      Find_UIObject(const _wstring& strUITag);
    CUIControllerBase*                              Find_UIController(const _wstring& strControllerTag);
    SpriteFont*                                     Find_UIFont(const string& strFontAssetID);
    HRESULT                                         Set_UIImageTexture(const UI_HANDLE& Handle, const _wstring& strRuntimeElementTag, const string& strTexturePath);

    HRESULT                                         Create_UIObjectFromAsset(const string& strAssetID, const _wstring& strInstanceTag);

    HRESULT                                         Register_UIAsset(const _wstring& strFilePath, UI_ASSET_VALIDATION_RESULT* pOutValidation = nullptr, _wstring* pError = nullptr);
    HRESULT                                         Register_UIController(const UI_HANDLE& Handle, const _wstring& strControllerTag, CUIControllerBase* pController);
    HRESULT                                         Register_UIControllerFactory(const string& strControllerType, const UI_CONTROLLER_FACTORY& Factory);
    HRESULT                                         Register_UIAssetDirectory(const _wstring& strDirectoryPath, size_t* pOutLoadedCount = nullptr, _wstring* pError = nullptr);
    HRESULT                                         Register_UIFont(const string& strFontAssetID, const _wstring& strFontPath);


    HRESULT                                         Create_UIController(const UI_CONTROLLER_CREATE_CONTEXT& Context);

    const UI_ASSET_DESC*                            Find_UIAsset(const string& strAssetID) const;
    _bool                                           Contains_UIAsset(const string& strAssetID) const;
    size_t                                          Get_UIAssetCount() const;

    HRESULT                                         Open_UIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);
    HRESULT                                         Open_UIObject(const UI_HANDLE& Handle);
    HRESULT                                         Close_UIObject(const UI_HANDLE& Handle);

    void                                            Request_RemoveUIController(const _wstring& strControllerTag);
    void                                            Request_ReleaseUIObject(const UI_HANDLE& Handle);
    void                                            Request_ReleaseLifetime(UI_LIFETIME eLifetime);
    HRESULT                                         Request_SpawnUIObject(const string& strAssetID, const _wstring& strInstanceTag, UI_HANDLE* pOutHandle);
    HRESULT                                         Request_PushUIInputOwner(const UI_HANDLE& Handle, UI_INPUT_OWNER_KIND eKind);
    HRESULT                                         Request_PopUIInputOwner(const UI_HANDLE& Handle);
    
    _bool                                           Is_AliveUIObject(const UI_HANDLE& Handle) const;
    _bool                                           Is_OpenUIObject(const UI_HANDLE& Handle) const;
    _bool                                           Is_PendingSpawnUIObject(const UI_HANDLE& Handle) const;
    _bool                                           Contains_UIFont(const string& strFontAssetID) const;
    _bool                                           Has_UIInputOwner() const;
    _bool                                           Is_TopUIInputOwner(const UI_HANDLE& Handle) const;
    _bool                                           Can_PushUI(UI_INPUT_OWNER_KIND eKind) const;

    void                                            Acquire_UILayerSuppression(const string& strLayer);
    void                                            Release_UILayerSuppression(const string& strLayer);
    _bool                                           Is_UILayerSuppressed(const string& strLayer) const;

    void                                            Clear();

private:
    HRESULT                                         Create_Material(const string& strTexturePath, CMaterial** ppOutMaterial);
    HRESULT                                         Create_Material(const string& strTexturePath, const string& strDissolveTexturePath, CMaterial** ppOutMaterial);
    HRESULT                                         Create_UIObject(const UI_ASSET_DESC& AssetDesc, const _wstring& strInstanceTag);
    HRESULT                                         Create_UIRoot(CUIObject* pUIObject, const UI_ELEMENT_ASSET_DESC& RootDesc, const _float2& vRuntimeScale, unordered_set<string>& ExpansionStack);
    HRESULT                                         Create_UIElement(CUIObject* pUIObject, CUIElement* pParent, const UI_ELEMENT_ASSET_DESC& ElementDesc, const _float2& vRuntimeScale, const _wstring& strBindingPrefix, _uint iSortGroup, unordered_set<string>& ExpansionStack);
    HRESULT                                         Create_UIComponentInstance(CUIObject* pUIObject, CUIElement* pParent, const UI_ELEMENT_ASSET_DESC& ElementDesc, const _float2& vRuntimeScale, const _wstring& strBindingPrefix, _uint iSortGroup, unordered_set<string>& ExpansionStack);
    
    _wstring                                        Make_UIRuntimeElementTag(const UI_ELEMENT_ASSET_DESC& ElementDesc, const _wstring& strBindingPrefix) const;

    void                                            Apply_UITransform(CUIElement* pElement, const UITransform& Transform, const  _float2& vRuntimeScale);

    UI_HANDLE                                       Allocate_UIRuntimeSlot();
    void                                            Release_UIRuntimeSlot(const UI_HANDLE& Handle);
    _bool                                           Is_ValidUIHandle(const UI_HANDLE& Handle) const;

    _bool                                           Is_SingletonPolicy(UI_INSTANCE_POLICY ePolicy) const;
    _float2                                         Get_RuntimeUIScale(const UI_ASSET_DESC& AssetDesc);
    _wstring                                        Resolve_TexturePath(const string& strTexturePath);

    void                                            Remove_UIObjectImmediate(const UI_HANDLE& Handle);
    void                                            Remove_UIControllerImmediate(const _wstring& strControllerTag);

    void                                            Execute_SpawnUIObject(const UI_HANDLE& Handle);
    void                                            Cancel_PendingSpawn(const UI_HANDLE& Handle);
    void                                            Flush_PendingCommands();

private:
    _bool                                           Is_UIInputOwner(const UI_HANDLE& Handle) const;
    _bool                                           Has_PendingUIInputOwnerCommand(const UI_HANDLE& Handle) const;
    _bool                                           Has_UIInputOwnerKind(UI_INPUT_OWNER_KIND eKind) const;
    _bool                                           Has_PendingUIInputOwnerPushKind(UI_INPUT_OWNER_KIND eKind) const;
    _bool                                           Is_UIInputOwnerRenderSuppressed(const UI_HANDLE& Handle) const;

    void                                            Request_RemoveUIInputOwner(const UI_HANDLE& Handle);
    void                                            Remove_UIInputOwnerImmediate(const UI_HANDLE& Handle);
    void                                            Flush_UIInputOwnerCommands();
    void                                            Refresh_UIRenderSuppression();

private:
    CUIButton*                                      Find_TopButtonAtPoint(const _float2& vUIPoint, UI_HANDLE* pOutHandle) const;

    _bool                                           Is_ValidButtonTarget(const UI_HANDLE& Handle, const CUIButton* pButton) const;
    _bool                                           Is_ButtonOwnedByUI(const UI_HANDLE& Handle, const CUIButton* pButton) const;

    void                                            Update_ButtonPointerInput();
    void                                            Reset_ButtonPointerInput();
    void                                            Dispatch_ButtonCommand(const UI_HANDLE& Handle, const CUIButton* pButton);
    void                                            Dispatch_ButtonFocusEvent(const UI_HANDLE& Handle, const CUIButton* pButton, _bool bFocused);

    void                                            Set_FocusedButton(const UI_HANDLE& Handle, CUIButton* pButton);
    void                                            Clear_ButtonFocus();

private:
    ID3D11Device*                                   m_pDevice = { nullptr };
    ID3D11DeviceContext*                            m_pContext = { nullptr };
    CGameInstance*                                  m_pGameInstance = { nullptr };

    CUIAssetManager                                 m_UIAssetManager;
    CUIFontRegistry                                 m_UIFontRegistry;


    unordered_map<_wstring, CUIObject*>             m_UIObjects;                    // UIObject 실제 소유
    unordered_map<_wstring, CUIControllerBase*>     m_UIControllers;                // 컨트롤러 실제 소유

    vector<UI_RUNTIME_SLOT>                         m_UIRuntimeSlots;               // 반환되어 재사용 가능한 슬롯 인덱스
    vector<_uint>                                   m_FreeUIRuntimeSlotIndices;

    unordered_set<_wstring>                         m_PendingRemoveUIControllers;   // 컨트롤러 제거 예약

    vector<UI_RUNTIME_COMMAND>                      m_PendingUICommands;            // Spawn/Release 커맨드

    unordered_set<_wstring>                         m_PendingSpawnInstanceTags;     // 실제 생성 전 동일 인스턴스 태그가 예약되는 것을 차단

    unordered_map<string, UI_HANDLE>                m_SingletonUIHandles;           // 에셋 당 하나인 싱글톤 핸들

    unordered_map<string, UI_CONTROLLER_FACTORY>    m_UIControllerFactories;        // 컨트롤러타입 -> 게임 코어 컨트롤러 생성 함수
   
     // 동일 레이어를 여러 모달 UI가 숨길 수 있으므로  bool이 아니라 참조 횟수를 사용한다.
    unordered_map<string, _uint>                    m_UILayerSuppressionCounts;

private:
    vector<UI_INPUT_OWNER_ENTRY>                    m_UIInputOwnerStack;            // 확정된 입력 소유 순서, back이 현재 최상단 Owner
    vector<UI_INPUT_OWNER_COMMAND>                  m_PendingUIInputOwnerCommands;  // 다음 Late_Update에서 반영할 Push/Pop/Remove 요청

private:
    // 현재 마우스가 올라가 있는 Button
    UI_HANDLE                                       m_hHoveredUI = {};
    CUIButton*                                      m_pHoveredButton = { nullptr };

    // Mouse Down부터 Mouse Up까지 입력을 소유하는 Button
    UI_HANDLE                                       m_hCapturedUI = {};
    CUIButton*                                      m_pCapturedButton = { nullptr };

    // 현재 입력을 받을 Button
    UI_HANDLE                                       m_hFocusedUI = {};
    CUIButton*                                      m_pFocusedButton = { nullptr };

public:
    static CUI_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END
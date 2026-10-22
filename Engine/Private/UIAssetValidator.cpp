#include "UIAssetValidator.h"

namespace
{
    using namespace Engine;

    struct VALIDATION_CONTEXT
    {
        UI_ASSET_VALIDATION_RESULT* pResult = nullptr;

        // 에셋 전체에서 중복되면 안 되는 값
        unordered_set<_uint> ElementIDs;
        unordered_set<string> BindingKeys;
    };

    void Add_Error(VALIDATION_CONTEXT& Context, const _wstring& strPath, const _wstring& strMessage)
    {
        UI_ASSET_VALIDATION_ISSUE Issue = {};

        Issue.eSeverity = UI_ASSET_VALIDATION_SEVERITY::UI_ERROR;

        Issue.strPath = strPath;
        Issue.strMessage = strMessage;

        Context.pResult->Issues.push_back(move(Issue));
    }

    void Add_Warning(VALIDATION_CONTEXT& Context, const _wstring& strPath, const _wstring& strMessage)
    {
        UI_ASSET_VALIDATION_ISSUE Issue = {};

        Issue.eSeverity = UI_ASSET_VALIDATION_SEVERITY::UI_WARNING;

        Issue.strPath = strPath;
        Issue.strMessage = strMessage;

        Context.pResult->Issues.push_back(move(Issue));
    }

    // "__Element_", "__Runtime_" 등 런타임 내부 Tag와 충돌하는지 검사
    _bool Has_ReservedBindingPrefix(const string& strBindingKey)
    {
        return strBindingKey.size() >= 2 &&
            strBindingKey[0] == '_' &&
            strBindingKey[1] == '_';
    }

    // "."은 Component Binding Namespace 구분자로 사용
    _bool Has_BindingNamespaceSeparator(const string& strBindingKey)
    {
        return strBindingKey.find('.') != string::npos;
    }

    _bool Is_Finite(_float fValue)
    {
        return  std::isfinite(fValue);
    }

    _bool Is_Finite(const _float2& vValue)
    {
        return  Is_Finite(vValue.x) && 
                Is_Finite(vValue.y);
    }

    _bool Is_Finite(const _float4& vValue)
    {
        return  Is_Finite(vValue.x) && Is_Finite(vValue.y) && 
                Is_Finite(vValue.z) && Is_Finite(vValue.w);
    }
    
    void Validate_Image(const UI_ELEMENT_ASSET_DESC& Element, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        const UI_IMAGE_ASSET_DESC& Image = Element.Image;

        if (Element.Render.bUseTexture && Image.strTexturePath.empty())
            Add_Error(Context, strPath + L".Image.TexturePath", L"텍스쳐를 사용하는 Image의 경로가 비어 있습니다.");

        // DISSOLVE는 실제 이미지 외에 추가 노이즈 텍스처가 반드시 필요하다.
        if (Element.Render.eRevealType == UI_REVEAL_TYPE::DISSOLVE && Image.strDissolveTexturePath.empty())
            Add_Error(Context, strPath + L".Image.DissolveTexturePath", L"DISSOLVE Reveal의 노이즈 텍스처 경로가 비어 있습니다.");

        if (!Is_Finite(Image.vTextureSize) || Image.vTextureSize.x <= 0.f || Image.vTextureSize.y <= 0.f)
            Add_Error(Context, strPath + L".Image.TextureSize", L"TextureSize는 유한한 양수여야 합니다.");

        if (Image.eDrawType != UI_ASSET_IMAGE_DRAW_TYPE::NINE_SLICE)
            return;

        if (!Is_Finite(Image.vSliceBaseSize) || Image.vSliceBaseSize.x <= 0.f || Image.vSliceBaseSize.y <= 0.f)
            Add_Error(Context, strPath + L".Image.SliceBaseSize", L"NineSlice 기준 크기는 유한한 양수여야 합니다.");

        if (!Is_Finite(Image.vSliceMargin) ||
            Image.vSliceMargin.x < 0.f ||
            Image.vSliceMargin.y < 0.f ||
            Image.vSliceMargin.z < 0.f ||
            Image.vSliceMargin.w < 0.f)
        {
            Add_Error(Context, strPath + L".Image.SliceMargin", L"NineSlice 여백은 유한한 0 이상의 값이어야 합니다.");

            return;
        }

        if (Image.vSliceMargin.x + Image.vSliceMargin.z > Image.vSliceBaseSize.x)
            Add_Error(Context, strPath + L".Image.SliceMargin", L"좌우 여백 합이 기준 너비보다 큽니다.");

        if (Image.vSliceMargin.y + Image.vSliceMargin.w > Image.vSliceBaseSize.y)
            Add_Error(Context, strPath + L".Image.SliceMargin", L"상하 여백 합이 기준 높이보다 큽니다.");
    }

    void Validate_Progress(const UI_ELEMENT_ASSET_DESC& Element, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        const UI_PROGRESS_ASSET_DESC& Progress = Element.Progress;

        if (!Is_Finite(Progress.fLengthScale) || Progress.fLengthScale < 0.f)
            Add_Error(Context, strPath + L".Progress.LengthScale", L"LengthScale은 유한한 0 이상의 값이어야 합니다.");

        unordered_map<_int, UI_ASSET_PROGRESS_SLOT_TYPE> SlotTypes;

        // 같은 ProgressBar 내부의 Slot BindingKey 검사
        unordered_set<string> SlotBindingKeys;

        _bool bHasFillSlot = false;

        for (_uint i = 0; i < static_cast<_uint>(Progress.Slots.size()); ++i)
        {
            const UI_PROGRESS_SLOT_ASSET_DESC& Slot = Progress.Slots[i];

            const _wstring strSlotPath = strPath + L".Progress.Slots[" + to_wstring(i) + L"]";

            if (Slot.iSlotID < 0)
                Add_Error(Context, strSlotPath + L".SlotID", L"SlotID는 0 이상이어야 합니다.");
            else
            {
                const _bool bInserted = SlotTypes.emplace(Slot.iSlotID, Slot.eType).second;

                if (!bInserted)
                    Add_Error(Context, strSlotPath + L".BindingKey", L"ProgressBar 내부에서 중복된 Slot BindingKey입니다.");
            }

            if (Slot.eType == UI_ASSET_PROGRESS_SLOT_TYPE::FILL)
                bHasFillSlot = true;

            if (Slot.strName.empty())
                Add_Warning(Context, strSlotPath + L".Name", L"Progress Slot 이름이 비어 있습니다.");

            if (!Slot.strBindingKey.empty())
            {
                if (Has_ReservedBindingPrefix(Slot.strBindingKey))
                {
                    Add_Error(Context, strSlotPath + L".BindingKey", L"'__'로 시작하는 Slot BindingKey는 런타임 내부 예약어입니다.");
                }

                if (Has_BindingNamespaceSeparator(Slot.strBindingKey))
                {
                    Add_Error(Context, strSlotPath + L".BindingKey", L"Slot BindingKey에는 Namespace 구분자인 '.'을 사용할 수 없습니다.");
                }

                const _bool bInserted = SlotBindingKeys.insert(Slot.strBindingKey).second;

                if (!bInserted)
                {
                    Add_Error(Context, strSlotPath + L".BindingKey", L"같은 ProgressBar 내부에서 중복된 Slot BindingKey입니다.");
                }
            }

            if (Slot.iSortOffset < 0)
            {
                Add_Error(Context, strSlotPath + L".SortOffset", L"Progress Slot SortOffset은 0 이상이어야 합니다.");
            }
            else
            {
                const _uint iFinalLocalSort = Element.Render.iSortOrder + static_cast<_uint>(Slot.iSortOffset);

                if (iFinalLocalSort >= UI_ELEMENT_LOCAL_SORT_RANGE)
                {
                    Add_Error(Context, strSlotPath + L".SortOffset", L"Element SortOrder와 Slot SortOffset의 합은 999이하여야 합니다.");
                }
            }

            if (!Is_Finite(Slot.fAmount) || !Is_Finite(Slot.fStartAmount) || !Is_Finite(Slot.fEndAmount))
                Add_Error(Context, strSlotPath + L".Amount", L"Amount 값은 유한해야 합니다.");

            if (!Is_Finite(Slot.vUVRect))
                Add_Error(Context, strSlotPath + L".UVRect", L"UVRect에 유한하지 않은 값이 있습니다.");
            
            if (!Is_Finite(Slot.vColor) ||
                !Is_Finite(Slot.vColorLT) ||
                !Is_Finite(Slot.vColorRT) ||
                !Is_Finite(Slot.vColorRB) ||
                !Is_Finite(Slot.vColorLB))
            {
                Add_Error(Context, strSlotPath + L".Color", L"Slot Color에 유한하지 않은 값이 있습니다.");
            }
        }

        if (!bHasFillSlot)
            Add_Warning(Context, strPath + L".Progress.Slots", L"ProgressBar에 Fill 슬롯이 없습니다.");

        for (_uint i = 0; i < static_cast<_uint>(Progress.Slots.size()); ++i)
        {
            const UI_PROGRESS_SLOT_ASSET_DESC& Slot = Progress.Slots[i];

            const _wstring strSlotPath = strPath + L".Progress.Slots[" + to_wstring(i) + L"]";

            if (Slot.eType == UI_ASSET_PROGRESS_SLOT_TYPE::FX)
            {
                if (Slot.iTargetSlotID == -1)
                    continue;

                const auto Iter = SlotTypes.find(Slot.iTargetSlotID);

                if (Iter == SlotTypes.end())
                    Add_Error(Context, strSlotPath + L".TargetSlotID", L"FX가 존재하지 않는 슬롯을 참조합니다.");
                else if (Iter->second != UI_ASSET_PROGRESS_SLOT_TYPE::FILL)
                    Add_Error(Context, strSlotPath + L".TargetSlotID", L"FX 대상은 Fill 슬롯이어야 합니다.");
            }
            else if (Slot.iTargetSlotID != -1)
            {
                Add_Warning(Context, strSlotPath + L".TargetSlotID", L"FX가 아닌 슬롯의 TargetSlotID는 사용되지 않습니다.");
            }
        }
    }

    // Button 자신의 자식을 시작점으로 BindingKey를 재귀 검색
    const UI_ELEMENT_ASSET_DESC* FindButtonDescendantByBindingKey(const UI_ELEMENT_ASSET_DESC& ButtonElement, const string& strBindingKey)
    {
        for (const UI_ELEMENT_ASSET_DESC& Child : ButtonElement.Children)
        {
            if (Child.strBindingKey == strBindingKey)
                return &Child;

            const UI_ELEMENT_ASSET_DESC* pFound = FindButtonDescendantByBindingKey(Child, strBindingKey);

            if (pFound != nullptr)
                return pFound;
        }

        return nullptr;
    }

    // Button 안에 또 다른 Button이 들어갔는지 검사
    _bool HasNestedButton(const UI_ELEMENT_ASSET_DESC& Element)
    {
        for (const UI_ELEMENT_ASSET_DESC& Child : Element.Children)
        {
            if (Child.eType == UI_ELEMENT_TYPE::BUTTON)
                return true;

            if (HasNestedButton(Child))
                return true;
        }

        return false;
    }

    // 하나의 Button 시각 상태에 들어 있는 Override 목록 검증
    void ValidateButtonVisualState(const UI_ELEMENT_ASSET_DESC& ButtonElement, const UI_BUTTON_VISUAL_STATE_ASSET_DESC& State, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        unordered_set<string> TargetBindingKeys;

        for (_uint i = 0; i < static_cast<_uint>(State.Overrides.size()); ++i)
        {
            const UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC& Override = State.Overrides[i];

            const _wstring strOverridePath = strPath + L"[" + to_wstring(i) + L"]";

            if (Override.strTargetBindingKey.empty())
            {
                Add_Error(Context, strOverridePath + L".TargetBindingKey", L"Button 상태 Override의 대상 BindingKey가 비어 있습니다.");
            }
            else
            {
                const _bool bInserted = TargetBindingKeys.insert(Override.strTargetBindingKey).second;

                if (!bInserted)
                {
                    Add_Error(Context, strOverridePath + L".TargetBindingKey", L"같은 Button 상태에서 대상 BindingKey가 중복되었습니다.");
                }

                const UI_ELEMENT_ASSET_DESC* pTarget = FindButtonDescendantByBindingKey(ButtonElement, Override.strTargetBindingKey);

                if (pTarget == nullptr)
                {
                    Add_Error(Context, strOverridePath + L".TargetBindingKey", L"Button 하위에서 대상 BindingKey를 찾을 수 없습니다.");
                }
            }

            if (!Override.bOverrideVisible && !Override.bOverrideColor && !Override.bOverrideScale)
            {
                Add_Warning(Context, strOverridePath, L"활성화된 Button 상태 Override 속성이 없습니다.");
            }

            if (Override.bOverrideColor && !Is_Finite(Override.vColor))
            {
                Add_Error(Context, strOverridePath + L".Color", L"Button 상태 Color는 유한한 값이어야 합니다.");
            }

            if (Override.bOverrideScale && !Is_Finite(Override.vScale))
            {
                Add_Error(Context, strOverridePath + L".Scale", L"Button 상태 Scale은 유한한 값이어야 합니다.");
            }
        }
    }

    // Button 저장 데이터와 자식 상태 참조 검증
    void Validate_Button(const UI_ELEMENT_ASSET_DESC& Element, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        const UI_BUTTON_ASSET_DESC& Button = Element.Button;

        if (Button.strCommandKey.empty())
        {
            Add_Error(Context, strPath + L".Button.CommandKey", L"Button의 CommandKey가 비어 있습니다.");
        }
        else if (Has_ReservedBindingPrefix(Button.strCommandKey))
        {
            Add_Error(Context, strPath + L".Button.CommandKey", L"'__'로 시작하는 CommandKey는 런타임 내부 예약어입니다.");
        }

        if (Button.strFocusGroup.empty())
        {
            Add_Warning(Context, strPath + L".Button.FocusGroup", L"FocusGroup이 비어 있어 키보드, 게임패드 탐색에서 제외될 수 있습니다.");
        }

        if (Button.iFocusOrder < 0)
        {
            Add_Error(Context, strPath + L".Button.FocusOrder", L"FocusOrder는 0 이상이어야 합니다.");
        }

        if (Element.Children.empty())
        {
            Add_Warning(Context, strPath + L".Children", L"Button에 표시할 자식 Element가 없습니다.");
        }

        if (HasNestedButton(Element))
        {
            Add_Error(Context, strPath + L".Children", L"Button 안에는 다른 Button을 배치할 수 없습니다.");
        }

        ValidateButtonVisualState(Element, Button.Hovered, Context, strPath + L".Button.States.Hovered");
        ValidateButtonVisualState(Element, Button.Pressed, Context, strPath + L".Button.States.Pressed");
        ValidateButtonVisualState(Element, Button.Selected, Context, strPath + L".Button.States.Selected");
        ValidateButtonVisualState(Element, Button.Focused, Context, strPath + L".Button.States.Focused");
        ValidateButtonVisualState(Element, Button.Disabled, Context, strPath + L".Button.States.Disabled");
    }

    // ElementID로 에셋 원본 Element를 찾는다.
    const UI_ELEMENT_ASSET_DESC* FindElementByID(const UI_ELEMENT_ASSET_DESC& Element, _uint iElementID)
    {
        if (Element.iElementID == iElementID)
            return &Element;

        for (const UI_ELEMENT_ASSET_DESC& Child : Element.Children)
        {
            const UI_ELEMENT_ASSET_DESC* pFound = FindElementByID(Child, iElementID);

            if (pFound != nullptr)
                return pFound;
        }

        return nullptr;
    }

    // UI Asset이 보유한 Animation Clip 전체를 검증한다.
    void Validate_Animations(const UI_ASSET_DESC& Asset, VALIDATION_CONTEXT& Context)
    {
        unordered_set<string> ClipNames;

        for (_uint iClipIndex = 0; iClipIndex < static_cast<_uint>(Asset.Animations.size()); ++iClipIndex)
        {
            const UI_ANIMATION_CLIP_ASSET_DESC& Clip = Asset.Animations[iClipIndex];

            const _wstring strClipPath = L"Animations[" + to_wstring(iClipIndex) + L"]";

            if (Clip.strName.empty())
            {
                Add_Error(Context, strClipPath + L".Name", L"Animation Clip 이름이 비어 있습니다.");
            }
            else if (!ClipNames.insert(Clip.strName).second)
            {
                Add_Error(Context, strClipPath + L".Name", L"중복된 Animation Clip 이름입니다.");
            }

            if (!Is_Finite(Clip.fDuration) || Clip.fDuration <= 0.f)
                Add_Error(Context, strClipPath + L".Duration", L"Animation Clip 길이는 유한한 0보다 큰 값이어야 합니다.");

            // 같은 Element 속성을 한 Clip에서 두 Track이 동시에 제어하지 못하게 한다.
            unordered_set<_uint64> TrackKeys;

            for (_uint iTrackIndex = 0; iTrackIndex < static_cast<_uint>(Clip.Tracks.size()); ++iTrackIndex)
            {
                const UI_ANIMATION_TRACK_ASSET_DESC& Track = Clip.Tracks[iTrackIndex];

                const _wstring strTrackPath = strClipPath + L".Tracks[" + to_wstring(iTrackIndex) + L"]";

                if (FindElementByID(Asset.Root, Track.iTargetElementID) == nullptr)
                    Add_Error(Context, strTrackPath + L".TargetElementID", L"Animation Track 대상 ElementID가 존재하지 않습니다.");

                const _uint64 iTrackKey =
                    (static_cast<_uint64>(Track.iTargetElementID) << 32) |
                    static_cast<_uint64>(Track.eProperty);

                if (!TrackKeys.insert(iTrackKey).second)
                    Add_Error(Context, strTrackPath, L"같은 Element와 Property를 제어하는 Animation Track이 중복되었습니다.");

                if (Track.Keyframes.empty())
                {
                    Add_Warning(Context, strTrackPath + L".Keyframes", L"Animation Track에 Keyframe이 없습니다.");
                    continue;
                }

                _float fPreviousTime = -1.f;

                for (_uint iKeyIndex = 0; iKeyIndex < static_cast<_uint>(Track.Keyframes.size()); ++iKeyIndex)
                {
                    const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe = Track.Keyframes[iKeyIndex];

                    const _wstring strKeyPath = strTrackPath + L".Keyframes[" + to_wstring(iKeyIndex) + L"]";

                    if (!Is_Finite(Keyframe.fTime))
                        Add_Error(Context, strKeyPath + L".Time", L"Keyframe 시간은 유한한 값이어야 합니다.");
                    else
                    {
                        if (Keyframe.fTime < 0.f || Keyframe.fTime > Clip.fDuration)
                            Add_Error(Context, strKeyPath + L".Time", L"Keyframe 시간이 Animation Clip 범위를 벗어났습니다.");

                        if (Keyframe.fTime < fPreviousTime)
                            Add_Error(Context, strKeyPath + L".Time", L"Keyframe은 시간 오름차순으로 정렬되어야 합니다.");
                    }

                    if (!Is_Finite(Keyframe.vValue))
                        Add_Error(Context, strKeyPath + L".Value", L"Keyframe 값에 유한하지 않은 값이 있습니다.");

                    fPreviousTime = Keyframe.fTime;
                }
            }
        }
    }

    void Validate_Element(const UI_ELEMENT_ASSET_DESC& Element, VALIDATION_CONTEXT& Context, const _wstring& strPath)
    {
        // Element ID는 에셋 전체에서 고유해야 함
        const _bool bIDInserted = Context.ElementIDs.insert(Element.iElementID).second;

        if (!bIDInserted)
            Add_Error(Context, strPath + L".ID", L"중복된 Element ID입니다.");

        if (Element.strName.empty())
            Add_Warning(Context, strPath + L".Name", L"Element 표시 이름이 비어 있습니다.");

        if (Element.Render.iSortOrder >= UI_ELEMENT_LOCAL_SORT_RANGE)
        {
            Add_Error(Context, strPath + L".Render.SortOrder", L"Element SortOrder는 0 이상 999 이하여야 합니다.");
        }

        if (!Element.strBindingKey.empty())
        {
            if (Has_ReservedBindingPrefix(Element.strBindingKey))
            {
                Add_Error(Context, strPath + L".BindingKey", L"'__'로 시작하는 BindingKey는 런타임 내부 예약어입니다.");
            }

            if (Has_BindingNamespaceSeparator(Element.strBindingKey))
            {
                Add_Error(Context, strPath + L".BindingKey", L"BindingKey에는 컴포넌트 Namespace 구분자인 '.'을 사용할 수 없습니다.");
            }

            const _bool bBindingInserted = Context.BindingKeys.insert(Element.strBindingKey).second;

            if (!bBindingInserted)
            {
                Add_Error(Context, strPath + L".BindingKey", L"중복된 BindingKey입니다.");
            }
        }       
        
        const UITransform& Transform = Element.Transform;

        if (!Is_Finite(Transform.vPosition))
            Add_Error(Context, strPath + L".Transform.Position", L"Position에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Transform.vSize) || Transform.vSize.x < 0.f || Transform.vSize.y < 0.f)
            Add_Error(Context, strPath + L".Transform.Size", L"Size는 유한한 0 이상의 값이어야 합니다.");

        if (Element.eType != UI_ELEMENT_TYPE::CONTAINER && (Transform.vSize.x == 0.f || Transform.vSize.y == 0.f))
            Add_Warning(Context, strPath + L".Transform.Size", L"표시 Element의 크기가 0입니다.");

        if (!Is_Finite(Transform.vPivot))
            Add_Error(Context, strPath + L".Transform.Pivot", L"Pivot에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Transform.fRotation))
            Add_Error(Context, strPath + L".Transform.Rotation", L"Rotation은 유한한 값이어야 합니다.");

        if (!Is_Finite(Transform.vScale))
            Add_Error(Context, strPath + L".Transform.Scale", L"Scale에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Element.Render.vColor))
            Add_Error(Context, strPath + L".Render.Color", L"Color에 유한하지 않은 값이 있습니다.");

        if (!Is_Finite(Element.Render.vUVRect))
            Add_Error(Context, strPath + L".Render.UVRect", L"UVRect에 유한하지 않은 값이 있습니다.");

        // 현재 DISSOLVE는 두 번째 텍스처를 보유할 수 있는 Image에서만 지원한다.
        if (Element.Render.eRevealType == UI_REVEAL_TYPE::DISSOLVE && Element.eType != UI_ELEMENT_TYPE::IMAGE)
            Add_Error(Context, strPath + L".Render.RevealType", L"DISSOLVE Reveal은 Image Element에서만 사용할 수 있습니다.");

        if (Element.eType == UI_ELEMENT_TYPE::IMAGE)
            Validate_Image(Element, Context, strPath);
        else if (Element.eType == UI_ELEMENT_TYPE::PROGRESS_BAR)
            Validate_Progress(Element, Context, strPath);
        else if (Element.eType == UI_ELEMENT_TYPE::TEXT)
        {
            const UI_TEXT_ASSET_DESC& Text = Element.Text;

            if (Text.strFontAssetID.empty())
            {
                Add_Error(Context, strPath + L".Text.FontAssetID", L"Text의 FontAssetID가 비어 있습니다.");
            }

            if (!Is_Finite(Text.fFontSize) || Text.fFontSize <= 0.f)
            {
                Add_Error(Context, strPath + L".Text.FontSize", L"FontSize는 유한한 0보다 큰 값이어야 합니다.");
            }

            if (!Is_Finite(Text.fLineSpacing) || Text.fLineSpacing <= 0.f)
            {
                Add_Error(Context, strPath + L".Text.LineSpacing", L"LineSpacing은 유한한 0보다 큰 값이어야 합니다.");
            }

            // Text는 단독으로 렌더링되는 Leaf Element
            if (!Element.Children.empty())
            {
                Add_Error(Context, strPath + L".Children", L"Text Element는 직접 자식을 가질 수 없습니다.");
            }
        }
        else if (Element.eType == UI_ELEMENT_TYPE::BUTTON)
        {
            Validate_Button(Element, Context, strPath);
        }
        else if (Element.eType == UI_ELEMENT_TYPE::COMPONENT_INSTANCE)
        {
            if (Element.ComponentInstance.strComponentAssetID.empty())
            {
                Add_Error(Context, strPath + L".ComponentInstance.AssetID", L"Component Instance의 참조 Asset ID가 비어 있습니다.");
            }

            // Component Instance는 참조 노드이므로 자신의 Children을 갖지 않음
            if (!Element.Children.empty())
            {
                Add_Error(Context, strPath + L".Children", L"Component Instance는 직접 자식을 가질 수 없습니다.");
            }
        }

        for (_uint i = 0; i < static_cast<_uint>(Element.Children.size()); ++i)
        {
            const _wstring strChildPath = strPath + L".Children[" + to_wstring(i) + L"]";

            Validate_Element(Element.Children[i], Context, strChildPath);
        }
    }
}


HRESULT CUIAssetValidator::Validate(const UI_ASSET_DESC& Asset, UI_ASSET_VALIDATION_RESULT* pOutResult)
{
    if (pOutResult == nullptr)
        return E_FAIL;

    pOutResult->Issues.clear();

    VALIDATION_CONTEXT Context = {};
    Context.pResult = pOutResult;

    if (Asset.iSchemaVersion != UI_SCHEMA_VERSION)
        Add_Error(Context, L"SchemaVersion", L"지원하지 않는 UI 스키마 버전입니다.");

    if (Asset.strAssetID.empty())
        Add_Error(Context, L"Asset.ID", L"Asset ID는 비어 있을 수 없습니다.");

    if (Asset.strName.empty())
        Add_Warning(Context, L"Asset.Name", L"Asset 표시 이름이 비어 있습니다.");

    if (Asset.strLayer.empty())
        Add_Error(Context, L"Asset.Layer", L"Asset Layer는 비어 있을 수 없습니다.");

    if (!Is_Finite(Asset.vReferenceResolution) || Asset.vReferenceResolution.x <= 0.f || Asset.vReferenceResolution.y <= 0.f)
        Add_Error(Context, L"Canvas.ReferenceResolution", L"기준 해상도는 유한한 양수여야 합니다.");

    if (!Is_Finite(Asset.fMatch) || Asset.fMatch < 0.f || Asset.fMatch > 1.f)
        Add_Error(Context, L"Canvas.Match", L"Canvas Match는 0과 1 사이여야 합니다.");

    if (Asset.iSortOrder > UINT_MAX / UI_ELEMENT_SORT_RANGE)
    {
        Add_Error(Context, L"Asset.SortOrder", L"Asset SortOrder가 런타임 정렬 범위를 초과합니다.");
    }

    if (Asset.eKind == UI_ASSET_KIND::PROJECTED)
    {
        if (!Is_Finite(Asset.Projected.vScreenOffset))
            Add_Error(Context, L"Projected.ScreenOffset", L"화면 오프셋은 유한한 값이어야 합니다.");

        if (Asset.Projected.strAnchorSource.empty())
            Add_Warning(Context, L"Projected.AnchorSource", L"Projected UI의 AnchorSource가 비어 있습니다.");
    }

    Validate_Element(Asset.Root, Context, L"Root");
    Validate_Animations(Asset, Context);

    return pOutResult->Has_Error() ? E_FAIL : S_OK;
}

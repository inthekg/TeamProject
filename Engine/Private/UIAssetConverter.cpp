#include "UIAssetConverter.h"
#include "UI_Function.h"

namespace
{
    using namespace Engine;

    HRESULT Convert_Element(const json& ElementJson, UI_ELEMENT_ASSET_DESC* pOutElement, const _wstring& strPath, _wstring* pError)
    {
        if (pOutElement == nullptr)
            return E_FAIL;

        if (!ElementJson.is_object())
            return Fail(pError, strPath + L": Element must be an object.");

        UI_ELEMENT_ASSET_DESC Element = {};

        const _int iLegacyID = ElementJson.value("ID", -1);

        if (iLegacyID < 0)
            return Fail(pError, strPath + L": Invalid Element ID.");

        Element.iElementID = static_cast<_uint>(iLegacyID);

        Element.strName = ElementJson.value("Name", string());

        Element.strBindingKey = ElementJson.value("BindingKey", string());

        // v1 BindingKey가 비어 있으면 Name을 기본 키로 사용
        if (Element.strBindingKey.empty())
            Element.strBindingKey = Element.strName;

        const string strType = ElementJson.value("Type", string());

        if (strType == "Image")
            Element.eType = UI_ELEMENT_TYPE::IMAGE;
        else if (strType == "ProgressBar")
            Element.eType = UI_ELEMENT_TYPE::PROGRESS_BAR;
        else
            return Fail(pError, strPath + L": Unsupported Element Type.");

        if (ElementJson.contains("Transform"))
        {
            if (!ElementJson["Transform"].is_object())
                return Fail(pError, strPath + L": Transform must be an object.");

            const json& TransformJson = ElementJson["Transform"];

            if (TransformJson.contains("Position") && !ReadFloat2(TransformJson["Position"], &Element.Transform.vPosition))
                return Fail(pError, strPath + L".Transform.Position is invalid.");

            if (TransformJson.contains("Size") && !ReadFloat2(TransformJson["Size"], &Element.Transform.vSize))
                return Fail(pError, strPath + L".Transform.Size is invalid.");

            if (TransformJson.contains("Pivot") && !ReadFloat2(TransformJson["Pivot"], &Element.Transform.vPivot))
                return Fail(pError, strPath + L".Transform.Pivot is invalid.");

            const _int iAnchor = TransformJson.value("Anchor", 0);

            if (iAnchor < 0 || iAnchor > 8)
                return Fail(pError, strPath + L".Transform.Anchor is invalid.");

            Element.Transform.eAnchor = static_cast<UI_ANCHOR>(iAnchor);
            Element.Transform.fRotation = TransformJson.value("Rotation", 0.f);

            if (TransformJson.contains("Scale") && !ReadFloat2(TransformJson["Scale"], &Element.Transform.vScale))
                return Fail(pError, strPath + L".Transform.Scale is invalid.");
        }

        if (ElementJson.contains("RenderData"))
        {
            if (!ElementJson["RenderData"].is_object())
                return Fail(pError, strPath + L": RenderData must be an object.");

            const json& RenderJson = ElementJson["RenderData"];

            if (RenderJson.contains("Color") && !ReadFloat4(RenderJson["Color"], &Element.Render.vColor))
                return Fail(pError, strPath + L".RenderData.Color is invalid.");

            if (RenderJson.contains("UVRect") && !ReadFloat4(RenderJson["UVRect"], &Element.Render.vUVRect))
                return Fail(pError, strPath + L".RenderData.UVRect is invalid.");

            Element.Render.iSortOrder = RenderJson.value("SortOrder", 0u);
            Element.Render.bVisible = RenderJson.value("Visible", true);
            Element.Render.bUseTexture = RenderJson.value("UseTexture", true);
        }

        if (Element.eType == UI_ELEMENT_TYPE::IMAGE)
        {
            if (!ElementJson.contains("Image") || !ElementJson["Image"].is_object())
                return Fail(pError, strPath + L": Image block is missing.");

            const json& ImageJson = ElementJson["Image"];

            Element.Image.strTexturePath = ImageJson.value("TexturePath", string());

            const _int iDrawType = ImageJson.value("ImageDrawType", 0);

            if (iDrawType < 0 || iDrawType > 1)
                return Fail(pError, strPath + L".Image.ImageDrawType is invalid.");

            Element.Image.eDrawType = static_cast<UI_ASSET_IMAGE_DRAW_TYPE>(iDrawType);
            // v1 로더와 동일한 기본값
            Element.Image.vSliceBaseSize = Element.Transform.vSize;

            if (ImageJson.contains("SliceMargin") && !ReadFloat4(ImageJson["SliceMargin"], &Element.Image.vSliceMargin))
                return Fail(pError, strPath + L".Image.SliceMargin is invalid.");

            if (ImageJson.contains("TextureSize") && !ReadFloat2(ImageJson["TextureSize"], &Element.Image.vTextureSize))
                return Fail(pError, strPath + L".Image.TextureSize is invalid.");

            if (ImageJson.contains("SliceBaseSize") && !ReadFloat2(ImageJson["SliceBaseSize"], &Element.Image.vSliceBaseSize))
                return Fail(pError, strPath + L".Image.SliceBaseSize is invalid.");

            // v1 TextureKey는 변환 X
            // TexturePath를 v2의 리소스 식별자로 사용
        }
        else if (Element.eType == UI_ELEMENT_TYPE::PROGRESS_BAR)
        {
            if (!ElementJson.contains("Progress") || !ElementJson["Progress"].is_object())
                return Fail(pError, strPath + L": Progress block is missing.");

            const json& ProgressJson = ElementJson["Progress"];

            Element.Progress.fLengthScale = ProgressJson.value("LengthScale", 1.f);

            const _int iFillType = ProgressJson.value("FillType", 0);
            const _int iFillDirection = ProgressJson.value("FillDirection", 0);
            const _int iResizeDirection = ProgressJson.value("ResizeDirection", 0);

            if (iFillType < 0 || iFillType > 2)
                return Fail(pError, strPath + L".Progress.FillType is invalid.");

            if (iFillDirection < 0 || iFillDirection > 5)
                return Fail(pError, strPath + L".Progress.FillDirection is invalid.");

            if (iResizeDirection < 0 || iResizeDirection > 4)
                return Fail(pError, strPath + L".Progress.ResizeDirection is invalid.");

            Element.Progress.eFillType = static_cast<UI_PROGRESS_FILL_TYPE>(iFillType);
            Element.Progress.eFillDirection = static_cast<UI_PROGRESS_FILL_DIRECTION>(iFillDirection);
            Element.Progress.eResizeDirection = static_cast<UI_PROGRESS_RESIZE_DIRECTION>(iResizeDirection);

            if (!ProgressJson.contains("Slots") || !ProgressJson["Slots"].is_array())
                return Fail(pError, strPath + L".Progress.Slots must be an array.");

            const json& SlotsJson = ProgressJson["Slots"];

            for (_uint i = 0; i < static_cast<_uint>(SlotsJson.size()); ++i)
            {
                const json& SlotJson = SlotsJson[i];

                const _wstring strSlotPath = strPath + L".Progress.Slots[" + to_wstring(i) + L"]";

                if (!SlotJson.is_object())
                    return Fail(pError, strSlotPath + L": Slot must be an object.");

                UI_PROGRESS_SLOT_ASSET_DESC Slot = {};

                Slot.iSlotID = SlotJson.value("SlotID", -1);
                Slot.iTargetSlotID = SlotJson.value("TargetSlotID", -1);
                Slot.strName = SlotJson.value("Name", string("Slot"));
                // v1 Slot에 BindingKey 없었으니 Name 사용
                Slot.strBindingKey = Slot.strName;
                Slot.strTexturePath = SlotJson.value("TexturePath", string());

                const _int iSlotType = SlotJson.value("Type", 1);
                const _int iBlendMode = SlotJson.value("BlendMode", 0);

                if (iSlotType < 0 || iSlotType > 2)
                    return Fail(pError, strSlotPath + L".Type is invalid.");

                if (iBlendMode < 0 || iBlendMode > 1)
                    return Fail(pError, strSlotPath + L".BlendMode is invalid.");

                Slot.eType = static_cast<UI_ASSET_PROGRESS_SLOT_TYPE>(iSlotType);
                Slot.eBlendMode = static_cast<UI_ASSET_PROGRESS_BLEND_MODE>(iBlendMode);
                Slot.fAmount = SlotJson.value("Amount", 1.f);
                Slot.fStartAmount = SlotJson.value("StartAmount", 0.f);
                Slot.fEndAmount = SlotJson.value("EndAmount", Slot.fAmount);
                Slot.bStretchUVToRange = SlotJson.value("StretchUVToRange", false);

                if (SlotJson.contains("UVRect") && !ReadFloat4(SlotJson["UVRect"], &Slot.vUVRect))
                    return Fail(pError, strSlotPath + L".UVRect is invalid.");

                if (SlotJson.contains("Color") && !ReadFloat4(SlotJson["Color"], &Slot.vColor))
                    return Fail(pError, strSlotPath + L".Color is invalid.");

                // v1 로더처럼 개별 색상이 없으면 기본 Color 사용
                Slot.vColorLT = Slot.vColor;
                Slot.vColorRT = Slot.vColor;
                Slot.vColorRB = Slot.vColor;
                Slot.vColorLB = Slot.vColor;

                if (SlotJson.contains("ColorLT") && !ReadFloat4(SlotJson["ColorLT"], &Slot.vColorLT))
                    return Fail(pError, strSlotPath + L".ColorLT is invalid.");

                if (SlotJson.contains("ColorRT") && !ReadFloat4(SlotJson["ColorRT"], &Slot.vColorRT))
                    return Fail(pError, strSlotPath + L".ColorRT is invalid.");

                if (SlotJson.contains("ColorRB") && !ReadFloat4(SlotJson["ColorRB"], &Slot.vColorRB))
                    return Fail(pError, strSlotPath + L".ColorRB is invalid.");

                if (SlotJson.contains("ColorLB") && !ReadFloat4(SlotJson["ColorLB"], &Slot.vColorLB))
                    return Fail(pError, strSlotPath + L".ColorLB is invalid.");

                Slot.iSortOffset = SlotJson.value("SortOffset", 0);
                Slot.bVisible = SlotJson.value("Visible", true);

                Element.Progress.Slots.push_back(move(Slot));
            }
        }

        *pOutElement = move(Element);

        return S_OK;
    }

   
    HRESULT Convert_Object(const json& ObjectJson, const _float2& vCanvasSize, _float fCanvasMatch, UI_ASSET_DESC* pOutAsset, UI_ASSET_VALIDATION_RESULT* pDiagnostics, const _wstring& strPath, _wstring* pError)
    {
        if (pOutAsset == nullptr || pDiagnostics == nullptr)
            return E_FAIL;

        UI_ASSET_DESC Asset = {};

        Asset.iSchemaVersion = UI_SCHEMA_VERSION;

        Asset.strName = ObjectJson.value("Name", string());

        Asset.strAssetID = Asset.strName;

        if (Asset.strAssetID.empty())
            return Fail(pError, strPath + L": Object Name is empty.");

        Asset.iSortOrder = ObjectJson.value("SortOrder", 0u);

        Asset.vReferenceResolution = vCanvasSize;

        Asset.fMatch = fCanvasMatch;

        // v1에는 이 정보를 저장하지 않았으므로 안전한 기본값 사용
        Asset.eKind = UI_ASSET_KIND::SCREEN;

        Asset.strLayer = "HUD";

        Asset.eInstancePolicy = UI_INSTANCE_POLICY::SINGLETON;

        Asset.eLifetime = UI_LIFETIME::GAMEPLAY;

        Asset.strControllerType.clear();

        // 변환 후 사용자가 직접 확인해야 하는 값
        UI_ASSET_VALIDATION_ISSUE MetadataWarning = {};

        MetadataWarning.eSeverity = UI_ASSET_VALIDATION_SEVERITY::UI_WARNING;

        MetadataWarning.strPath = strPath + L".Asset";

        MetadataWarning.strMessage = L"Kind, Layer, InstancePolicy, Lifetime, ControllerType을 수동으로 확인해야 합니다.";

        pDiagnostics->Issues.push_back(move(MetadataWarning));

        Asset.Root.eType = UI_ELEMENT_TYPE::CONTAINER;

        Asset.Root.strName = "Root";
        Asset.Root.strBindingKey.clear();

        Asset.Root.Transform.vPosition = _float2(0.f, 0.f);
        Asset.Root.Transform.vSize = vCanvasSize;
        Asset.Root.Transform.vPivot = _float2(0.5f, 0.5f);
        Asset.Root.Transform.eAnchor = UI_ANCHOR::CENTER;
        Asset.Root.Transform.fRotation = 0.f;
        Asset.Root.Transform.vScale = _float2(1.f, 1.f);

        Asset.Root.Render.bVisible = true;
        Asset.Root.Render.bUseTexture = false;

        if (!ObjectJson.contains("Elements") || !ObjectJson["Elements"].is_array())
            return Fail(pError, strPath + L": Elements array is missing.");

        const json& ElementsJson = ObjectJson["Elements"];

        unordered_set<_uint> UsedElementIDs;

        _uint iNextElementID = 0;

        for (const json& ElementJson : ElementsJson)
        {
            if (!ElementJson.is_object())
                return Fail(pError, strPath + L": Element must be an object.");
            const _int iLegacyID = ElementJson.value("ID", -1);

            if (iLegacyID >= 0)
            {
                UsedElementIDs.insert(static_cast<_uint>(iLegacyID));
                
                if (static_cast<_uint>(iLegacyID) >= iNextElementID)
                    iNextElementID = static_cast<_uint>(iLegacyID) + 1;
            }
        }

        // 기존 ID와 겹치지 않는 Root ID 생성
        while (UsedElementIDs.find(iNextElementID) != UsedElementIDs.end())
        {
            ++iNextElementID;
        }

        Asset.Root.iElementID = iNextElementID;

        for (_uint i = 0; i < static_cast<_uint>(ElementsJson.size()); ++i)
        {
            UI_ELEMENT_ASSET_DESC Element = {};
            
            const _wstring strElementPath = strPath + L".Elements[" + to_wstring(i) + L"]";

            if (FAILED(Convert_Element(ElementsJson[i], &Element, strElementPath, pError)))
                return E_FAIL;

            const _float2 vAnchorOffset = Calc_UIAnchorOffset(Element.Transform.eAnchor, Asset.Root.Transform.vSize, Asset.Root.Transform.vPivot);

            Element.Transform.vPosition.x -= vAnchorOffset.x;
            Element.Transform.vPosition.y -= vAnchorOffset.y;

            Asset.Root.Children.push_back(move(Element));
        }

        *pOutAsset = move(Asset);

        return S_OK;
    }
}

_bool CUIAssetConverter::Is_V1Document(const json& LegacyRoot)
{
    if (!LegacyRoot.is_object())
        return false;

    if (LegacyRoot.contains("SchemaVersion"))
        return false;

    if (!LegacyRoot.contains("Objects") || !LegacyRoot["Objects"].is_array())
        return false;

    return true;
}

HRESULT CUIAssetConverter::Convert_V1(const json& LegacyRoot, UI_ASSET_CONVERSION_RESULT* pOutResult, _wstring* pError)
{
    if (pOutResult == nullptr)
        return E_FAIL;

    if (!Is_V1Document(LegacyRoot))
        return Fail(pError, L"입력 JSON이 v1 UI 문서가 아닙니다.");

    try
    {
        UI_ASSET_CONVERSION_RESULT Converted = {};

        _float2 vCanvasSize = { 1280.f, 720.f };
        _float fCanvasMatch = 0.5f;

        if (LegacyRoot.contains("Canvas"))
        {
            if (!LegacyRoot["Canvas"].is_object())
                return Fail(pError, L"v1 Canvas must be an object.");

            const json& CanvasJson = LegacyRoot["Canvas"];

            if (CanvasJson.contains("Size"))
            {
                if (!ReadFloat2(CanvasJson["Size"], &vCanvasSize))
                    return Fail(pError, L"Invalid v1 Canvas.Size.");
            }

            fCanvasMatch = CanvasJson.value("Match", 0.5f);
        }

        const json& ObjectsJson = LegacyRoot["Objects"];

        unordered_set<string> ConvertedAssetIDs;

        for (_uint i = 0; i < static_cast<_uint>(ObjectsJson.size()); ++i)
        {
            if (!ObjectsJson[i].is_object())
                return Fail(pError, L"v1 Object must be an object.");

            UI_ASSET_DESC Asset = {};

            const _wstring strObjectPath = L"Objects[" + to_wstring(i) + L"]";

            if (FAILED(Convert_Object(ObjectsJson[i], vCanvasSize, fCanvasMatch, &Asset, &Converted.Diagnostics, strObjectPath, pError)))
                return E_FAIL;

            const _bool bAssetIDInserted = ConvertedAssetIDs.insert(Asset.strAssetID).second;

            if (!bAssetIDInserted)
                return Fail(pError, strObjectPath + L": 중복된 Asset ID입니다.");

            UI_ASSET_VALIDATION_RESULT Validation = {};

            if (FAILED(CUIAssetValidator::Validate(Asset, &Validation)))
            {
                // 검증 결과를 변환 진단 목록에 추가
                for (auto& Issue : Validation.Issues)
                {
                    Issue.strPath = strObjectPath + L"." + Issue.strPath;

                    Converted.Diagnostics.Issues.push_back(move(Issue));
                }

                pOutResult->Assets.clear();
                pOutResult->Diagnostics = move(Converted.Diagnostics);

                return Fail(pError, L"변환된 v2 Asset 검증에 실패했습니다.");
            }

            // Warning도 결과에 보존
            for (auto& Issue : Validation.Issues)
            {
                Issue.strPath = strObjectPath + L"." + Issue.strPath;

                Converted.Diagnostics.Issues.push_back(move(Issue));
            }

            Converted.Assets.push_back(move(Asset));
        }

        *pOutResult = move(Converted);

        return S_OK;
    }
    catch (const json::exception&)
    {
        return Fail(pError, L"v1 UI JSON 변환 중 타입 오류가 발생했습니다.");
    }
    catch (const exception&)
    {
        return Fail(pError, L"v1 UI 변환 중 예외가 발생했습니다.");
    }
}

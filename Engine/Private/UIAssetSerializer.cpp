#include "UIAssetSerializer.h"
#include "UI_Function.h"

namespace
{
	// 하나의 Button 상태 Override 목록을 JSON 배열로 변환
	json WriteButtonVisualState(const UI_BUTTON_VISUAL_STATE_ASSET_DESC& State)
	{
		json StateJson = json::array();

		for (const UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC& Override : State.Overrides)
		{
			json OverrideJson = {};

			OverrideJson["TargetBindingKey"] = Override.strTargetBindingKey;
			OverrideJson["OverrideVisible"] = Override.bOverrideVisible;
			OverrideJson["Visible"] = Override.bVisible;
			OverrideJson["OverrideColor"] = Override.bOverrideColor;
			OverrideJson["Color"] = WriteFloat4(Override.vColor);
			OverrideJson["OverrideScale"] = Override.bOverrideScale;
			OverrideJson["Scale"] = WriteFloat2(Override.vScale);

			StateJson.push_back(move(OverrideJson));
		}

		return StateJson;
	}

	// JSON 배열에서 하나의 Button 상태 Override 목록 복원
	HRESULT ReadButtonVisualState(const json& StateJson, UI_BUTTON_VISUAL_STATE_ASSET_DESC* pOutState, _wstring* pError, const _wstring& strPath)
	{
		if (pOutState == nullptr)
			return Fail(pError, strPath + L": Invalid output.");

		if (!StateJson.is_array())
			return Fail(pError, strPath + L" must be an array.");

		UI_BUTTON_VISUAL_STATE_ASSET_DESC State = {};

		_uint iOverrideIndex = 0;

		for (const json& OverrideJson : StateJson)
		{
			const _wstring strOverridePath = strPath + L"[" + to_wstring(iOverrideIndex) + L"]";

			if (!OverrideJson.is_object())
			{
				return Fail(pError, strOverridePath + L" must be an object.");
			}

			UI_BUTTON_CHILD_OVERRIDE_ASSET_DESC Override = {};

			Override.strTargetBindingKey = OverrideJson.value("TargetBindingKey", string());
			Override.bOverrideVisible = OverrideJson.value("OverrideVisible", false);
			Override.bVisible = OverrideJson.value("Visible", true);
			Override.bOverrideColor = OverrideJson.value("OverrideColor", false);

			if (!ReadFloat4(OverrideJson.value("Color", json::array()), &Override.vColor))
			{
				return Fail(pError, strOverridePath + L".Color is invalid.");
			}

			Override.bOverrideScale = OverrideJson.value("OverrideScale", false);

			if (!ReadFloat2(OverrideJson.value("Scale", json::array()), &Override.vScale))
			{
				return Fail(pError, strOverridePath + L".Scale is invalid.");
			}

			State.Overrides.push_back(move(Override));

			++iOverrideIndex;
		}

		*pOutState = move(State);

		return S_OK;
	}

	// Animation Clip 하나를 JSON으로 변환한다.
	json WriteAnimationClip(const UI_ANIMATION_CLIP_ASSET_DESC& Clip)
	{
		json ClipJson = {};

		ClipJson["Name"] = Clip.strName;
		ClipJson["Duration"] = Clip.fDuration;
		ClipJson["Loop"] = Clip.bLoop;
		ClipJson["Tracks"] = json::array();

		for (const UI_ANIMATION_TRACK_ASSET_DESC& Track : Clip.Tracks)
		{
			json TrackJson = {};

			TrackJson["TargetElementID"] = Track.iTargetElementID;
			TrackJson["Property"] = ToString(Track.eProperty);
			TrackJson["Keyframes"] = json::array();

			for (const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe : Track.Keyframes)
			{
				json KeyframeJson = {};

				KeyframeJson["Time"] = Keyframe.fTime;
				KeyframeJson["Value"] = WriteFloat4(Keyframe.vValue);
				KeyframeJson["Interpolation"] = ToString(Keyframe.eInterpolation);

				TrackJson["Keyframes"].push_back(move(KeyframeJson));
			}

			ClipJson["Tracks"].push_back(move(TrackJson));
		}

		return ClipJson;
	}

	// JSON에서 Animation Clip 하나를 복원한다.
	HRESULT ReadAnimationClip(const json& ClipJson, UI_ANIMATION_CLIP_ASSET_DESC* pOutClip, _wstring* pError, const _wstring& strPath)
	{
		if (pOutClip == nullptr)
			return Fail(pError, strPath + L": Invalid output.");

		if (!ClipJson.is_object())
			return Fail(pError, strPath + L" must be an object.");

		UI_ANIMATION_CLIP_ASSET_DESC Clip = {};

		Clip.strName = ClipJson.value("Name", string());
		Clip.fDuration = ClipJson.value("Duration", 1.f);
		Clip.bLoop = ClipJson.value("Loop", false);

		if (!ClipJson.contains("Tracks") || !ClipJson["Tracks"].is_array())
			return Fail(pError, strPath + L".Tracks must be an array.");

		_uint iTrackIndex = 0;

		for (const json& TrackJson : ClipJson["Tracks"])
		{
			const _wstring strTrackPath = strPath + L".Tracks[" + to_wstring(iTrackIndex) + L"]";

			if (!TrackJson.is_object())
				return Fail(pError, strTrackPath + L" must be an object.");

			UI_ANIMATION_TRACK_ASSET_DESC Track = {};

			Track.iTargetElementID = TrackJson.value("TargetElementID", UINT_MAX);

			const string strProperty = TrackJson.value("Property", string());

			if (!FromString(strProperty, &Track.eProperty))
				return Fail(pError, strTrackPath + L".Property is invalid.");

			if (!TrackJson.contains("Keyframes") || !TrackJson["Keyframes"].is_array())
				return Fail(pError, strTrackPath + L".Keyframes must be an array.");

			_uint iKeyframeIndex = 0;

			for (const json& KeyframeJson : TrackJson["Keyframes"])
			{
				const _wstring strKeyframePath = strTrackPath + L".Keyframes[" + to_wstring(iKeyframeIndex) + L"]";

				if (!KeyframeJson.is_object())
					return Fail(pError, strKeyframePath + L" must be an object.");

				UI_ANIMATION_KEYFRAME_ASSET_DESC Keyframe = {};

				Keyframe.fTime = KeyframeJson.value("Time", 0.f);

				if (!ReadFloat4(KeyframeJson.value("Value", json::array()), &Keyframe.vValue))
					return Fail(pError, strKeyframePath + L".Value is invalid.");

				const string strInterpolation = KeyframeJson.value("Interpolation", string("Linear"));

				if (!FromString(strInterpolation, &Keyframe.eInterpolation))
					return Fail(pError, strKeyframePath + L".Interpolation is invalid.");

				Track.Keyframes.push_back(move(Keyframe));

				++iKeyframeIndex;
			}

			Clip.Tracks.push_back(move(Track));

			++iTrackIndex;
		}

		*pOutClip = move(Clip);

		return S_OK;
	}
}

HRESULT CUIAssetSerializer::Save(const _wstring& strFilePath, const UI_ASSET_DESC& Asset, _wstring* pError)
{
	if (strFilePath.empty())
		return Fail(pError, L"UI Asset save path is empty.");

	try
	{
		const json Root = ToJson(Asset);
		const string strOutput = Root.dump(4);

		ofstream File(strFilePath, ios::out | ios::trunc | ios::binary);

		if (!File.is_open())
			return Fail(pError, L"Failed to open UI Asset file.");

		File.write(strOutput.data(), static_cast<streamsize>(strOutput.size()));

		if (!File.good())
			return Fail(pError, L"Failed to write UI Asset file.");

		return S_OK;
	}
	catch (const exception&)
	{
		return Fail(pError, L"UI Asset serialization failed.");
	}
}

HRESULT CUIAssetSerializer::Load(const _wstring& strFilePath, UI_ASSET_DESC* pOutAsset, _wstring* pError)
{
	if (pOutAsset == nullptr)
		return Fail(pError, L"Invalid UI Asset output.");

	if (strFilePath.empty())
		return Fail(pError, L"UI Asset load path is empty.");

	try
	{
		ifstream File(strFilePath, ios::in | ios::binary);

		if (!File.is_open())
			return Fail(pError, L"Failed to open UI Asset file.");

		json Root = json::parse(File, nullptr, true, true);

		UI_ASSET_DESC LoadedAsset = {};

		if (FAILED(FromJson(Root, &LoadedAsset, pError)))
			return E_FAIL;

		*pOutAsset = move(LoadedAsset);

		return S_OK;
	}
	catch (const exception&)
	{
		return Fail(pError, L"UI Asset parsing failed");
	}
}

// Asset 최상위 저장
json CUIAssetSerializer::ToJson(const UI_ASSET_DESC& Asset)
{
	json Root = {};

	Root["SchemaVersion"] = Asset.iSchemaVersion;

	Root["Asset"] = {};

	Root["Asset"]["ID"] = Asset.strAssetID;
	Root["Asset"]["Name"] = Asset.strName;
	Root["Asset"]["Kind"] = ToString(Asset.eKind);
	Root["Asset"]["Layer"] = Asset.strLayer;
	Root["Asset"]["SortOrder"] = Asset.iSortOrder;
	Root["Asset"]["InstancePolicy"] = ToString(Asset.eInstancePolicy);
	Root["Asset"]["Lifetime"] = ToString(Asset.eLifetime);
	Root["Asset"]["ControllerType"] = Asset.strControllerType;

	Root["Canvas"] = {};
	Root["Canvas"]["ReferenceResolution"] = WriteFloat2(Asset.vReferenceResolution);
	Root["Canvas"]["Match"] = Asset.fMatch;

	if (Asset.eKind == UI_ASSET_KIND::PROJECTED)
	{
		Root["Projected"] = {};
		
		Root["Projected"]["AnchorSource"] = Asset.Projected.strAnchorSource;
		Root["Projected"]["ScreenOffset"] = WriteFloat2(Asset.Projected.vScreenOffset);
		Root["Projected"]["HideOffscreen"] = Asset.Projected.bHideOffscreen;
		Root["Projected"]["UseDistanceScale"] = Asset.Projected.bUseDistanceScale;
	}

	Root["Root"] = Write_Element(Asset.Root);

	Root["Animations"] = json::array();

	for (const UI_ANIMATION_CLIP_ASSET_DESC& Clip : Asset.Animations)
	{
		Root["Animations"].push_back(WriteAnimationClip(Clip));
	}

	return Root;
}

// Asset 최상위 로드
HRESULT CUIAssetSerializer::FromJson(const json& Root, UI_ASSET_DESC* pOutAsset, _wstring* pError)
{
	if (pOutAsset == nullptr)
		return Fail(pError, L"Invalid UI Asset output.");

	try
	{
		if (!Root.is_object())
			return Fail(pError, L"UI Asset root must be an object.");

		if (!Root.contains("SchemaVersion") || !Root["SchemaVersion"].is_number_unsigned())
		{
			return Fail(pError, L"SchemaVersion is missing");
		}

		const _uint iVersion = Root["SchemaVersion"].get<_uint>();
		if (iVersion != UI_SCHEMA_VERSION)
			return Fail(pError, L"Unsupported UI Schema version.");

		if (!Root.contains("Asset") || !Root["Asset"].is_object())
		{
			return Fail(pError, L"Asset block is missing.");
		}

		if (!Root.contains("Root") || !Root["Root"].is_object())
		{
			return Fail(pError, L"Root Element is missing.");
		}

		UI_ASSET_DESC Asset = {};
		Asset.iSchemaVersion = iVersion;

		const json& AssetJson = Root["Asset"];

		Asset.strAssetID = AssetJson.value("ID", string());
		Asset.strName = AssetJson.value("Name", string());
		Asset.strControllerType = AssetJson.value("ControllerType", string());
		Asset.strLayer = AssetJson.value("Layer", string());
		Asset.iSortOrder = AssetJson.value("SortOrder", 0u);

		const string strKind = AssetJson.value("Kind", string());
		const string strPolicy = AssetJson.value("InstancePolicy", string());
		const string strLifetime = AssetJson.value("Lifetime", string());

		if (!FromString(strKind, &Asset.eKind))
			return Fail(pError, L"Invalid Asset Kind.");

		if (!FromString(strPolicy, &Asset.eInstancePolicy))
			return Fail(pError, L"Invalid Asset.InstancePolicy.");

		if (!FromString(strLifetime, &Asset.eLifetime))
			return Fail(pError, L"Invalid Asset.Lifetime.");

		if (!Root.contains("Canvas") || !Root["Canvas"].is_object())
			return Fail(pError, L"Root Canvas block is missing.");

		if (Root.contains("Canvas"))
		{
			const json& CanvasJson = Root["Canvas"];

			if (!ReadFloat2(CanvasJson.value("ReferenceResolution", json::array()), &Asset.vReferenceResolution))
				return Fail(pError, L"Invalid Canvas.ReferenceResolution.");

			Asset.fMatch = CanvasJson.value("Match", 0.5f);
		}

		if (Asset.eKind == UI_ASSET_KIND::PROJECTED)
		{
			if (!Root.contains("Projected") || !Root["Projected"].is_object())
				return Fail(pError, L"Projected block is missing.");

			const json& ProjectedJson = Root["Projected"];

			Asset.Projected.strAnchorSource = ProjectedJson.value("AnchorSource", string());

			if (!ReadFloat2(ProjectedJson.value("ScreenOffset", json::array()), &Asset.Projected.vScreenOffset))
				return Fail(pError, L"Invalid Projected.ScreenOffset.");

			Asset.Projected.bHideOffscreen = ProjectedJson.value("HideOffscreen", true);
			Asset.Projected.bUseDistanceScale = ProjectedJson.value("UseDistanceScale", false);
		}

		if (FAILED(Read_Element(Root["Root"], &Asset.Root, pError, L"Root", 0)))
			return E_FAIL;

		if (Root.contains("Animations"))
		{
			if (!Root["Animations"].is_array())
				return Fail(pError, L"Animations must be an array.");

			_uint iClipIndex = 0;

			for (const json& ClipJson : Root["Animations"])
			{
				UI_ANIMATION_CLIP_ASSET_DESC Clip = {};

				const _wstring strClipPath = L"Animations[" + to_wstring(iClipIndex) + L"]";

				if (FAILED(ReadAnimationClip(ClipJson, &Clip, pError, strClipPath)))
					return E_FAIL;

				Asset.Animations.push_back(move(Clip));

				++iClipIndex;
			}
		}

		*pOutAsset = move(Asset);

		return S_OK;
	}
	catch (const json::exception&)
	{
		return Fail(pError, L"Invalid JSON value or type.");
	}
	catch (const exception&)
	{
		return Fail(pError, L"UI Asset conversion failed.");
	}
}

// Element 저장
json CUIAssetSerializer::Write_Element(const UI_ELEMENT_ASSET_DESC& Element)
{
	json Result = {};

	Result["ID"] = Element.iElementID;
	Result["Name"] = Element.strName;
	Result["BindingKey"] = Element.strBindingKey;
	Result["Type"] = ToString(Element.eType);

	Result["Transform"] = {};
	Result["Transform"]["Position"] = WriteFloat2(Element.Transform.vPosition);
	Result["Transform"]["Size"] = WriteFloat2(Element.Transform.vSize);
	Result["Transform"]["Pivot"] = WriteFloat2(Element.Transform.vPivot);
	Result["Transform"]["Anchor"] = ToString(Element.Transform.eAnchor);
	Result["Transform"]["Rotation"] = Element.Transform.fRotation;
	Result["Transform"]["Scale"] = WriteFloat2(Element.Transform.vScale);

	Result["Render"] = {};
	Result["Render"]["Color"] = WriteFloat4(Element.Render.vColor);
	Result["Render"]["UVRect"] = WriteFloat4(Element.Render.vUVRect);
	Result["Render"]["SortOrder"] = Element.Render.iSortOrder;
	Result["Render"]["Visible"] = Element.Render.bVisible;
	Result["Render"]["UseTexture"] = Element.Render.bUseTexture;
	Result["Render"]["RevealType"] = ToString(Element.Render.eRevealType);
	Result["Render"]["RevealProgress"] = Element.Render.fRevealProgress;
	Result["Render"]["RevealSoftness"] = Element.Render.fRevealSoftness;
	Result["Render"]["RevealCenter"] = WriteFloat2(Element.Render.vRevealCenter);

	if (Element.eType == UI_ELEMENT_TYPE::IMAGE)
	{
		Result["Image"] = {};
		Result["Image"]["TexturePath"] = Element.Image.strTexturePath;
		Result["Image"]["DissolveTexturePath"] = Element.Image.strDissolveTexturePath;
		Result["Image"]["DrawType"] = ToString(Element.Image.eDrawType);
		Result["Image"]["SliceMargin"] = WriteFloat4(Element.Image.vSliceMargin);
		Result["Image"]["TextureSize"] = WriteFloat2(Element.Image.vTextureSize);
		Result["Image"]["SliceBaseSize"] = WriteFloat2(Element.Image.vSliceBaseSize);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::PROGRESS_BAR)
	{
		json ProgressJson = {};

		ProgressJson["LengthScale"] = Element.Progress.fLengthScale;

		ProgressJson["FillType"] = ToString(Element.Progress.eFillType);

		ProgressJson["FillDirection"] = ToString(Element.Progress.eFillDirection);

		ProgressJson["ResizeDirection"] = ToString(Element.Progress.eResizeDirection);

		ProgressJson["Slots"] = json::array();

		for (const auto& Slot : Element.Progress.Slots)
		{
			json SlotJson = {};

			SlotJson["SlotID"] = Slot.iSlotID;
			SlotJson["TargetSlotID"] = Slot.iTargetSlotID;

			SlotJson["Name"] = Slot.strName;
			SlotJson["BindingKey"] = Slot.strBindingKey;

			SlotJson["TexturePath"] = Slot.strTexturePath;

			SlotJson["Type"] = ToString(Slot.eType);

			SlotJson["BlendMode"] = ToString(Slot.eBlendMode);

			SlotJson["Amount"] = Slot.fAmount;
			SlotJson["StartAmount"] = Slot.fStartAmount;

			SlotJson["EndAmount"] = Slot.fEndAmount;

			SlotJson["UVRect"] = WriteFloat4(Slot.vUVRect);

			SlotJson["StretchUVToRange"] = Slot.bStretchUVToRange;

			SlotJson["Color"] = WriteFloat4(Slot.vColor);

			SlotJson["ColorLT"] = WriteFloat4(Slot.vColorLT);

			SlotJson["ColorRT"] = WriteFloat4(Slot.vColorRT);

			SlotJson["ColorRB"] = WriteFloat4(Slot.vColorRB);

			SlotJson["ColorLB"] = WriteFloat4(Slot.vColorLB);

			SlotJson["SortOffset"] = Slot.iSortOffset;

			SlotJson["Visible"] = Slot.bVisible;

			ProgressJson["Slots"].push_back(move(SlotJson));
		}

		Result["Progress"] = move(ProgressJson);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::TEXT)
	{
		json TextJson = {};

		TextJson["FontAssetID"] = Element.Text.strFontAssetID;
		TextJson["PreviewText"] = Element.Text.strPreviewText;
		TextJson["FontSize"] = Element.Text.fFontSize;
		TextJson["LineSpacing"] = Element.Text.fLineSpacing;
		TextJson["HorizontalAlign"] = ToString(Element.Text.eHorizontalAlign);
		TextJson["VerticalAlign"] = ToString(Element.Text.eVerticalAlign);
		TextJson["Wrap"] = Element.Text.bWrap;

		Result["Text"] = move(TextJson);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::BUTTON)
	{
		json ButtonJson = {};

		ButtonJson["CommandKey"] = Element.Button.strCommandKey;
		ButtonJson["Interactable"] = Element.Button.bInteractable;
		ButtonJson["FocusOnHover"] = Element.Button.bFocusOnHover;
		ButtonJson["SelectOnFocus"] = Element.Button.bSelectOnFocus;
		ButtonJson["CommandOnPress"] = Element.Button.bCommandOnPress;
		ButtonJson["FocusGroup"] = Element.Button.strFocusGroup;
		ButtonJson["FocusOrder"] = Element.Button.iFocusOrder;

		ButtonJson["States"] = {};
		ButtonJson["States"]["Hovered"] = WriteButtonVisualState(Element.Button.Hovered);
		ButtonJson["States"]["Pressed"] = WriteButtonVisualState(Element.Button.Pressed);
		ButtonJson["States"]["Selected"] = WriteButtonVisualState(Element.Button.Selected);
		ButtonJson["States"]["Focused"] = WriteButtonVisualState(Element.Button.Focused);
		ButtonJson["States"]["Disabled"] = WriteButtonVisualState(Element.Button.Disabled);

		Result["Button"] = move(ButtonJson);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::COMPONENT_INSTANCE)
	{
		Result["ComponentInstance"] = {};
		Result["ComponentInstance"]["AssetID"] = Element.ComponentInstance.strComponentAssetID;
	}

	Result["Children"] = json::array();

	for (const auto& Child : Element.Children)
	{
		Result["Children"].push_back(Write_Element(Child));
	}

	return Result;
}

HRESULT CUIAssetSerializer::Read_Element(const json& ElementJson, UI_ELEMENT_ASSET_DESC* pOutElement, _wstring* pError, const _wstring& strPath, _uint iDepth)
{
	if (pOutElement == nullptr)
		return Fail(pError, strPath + L": Invalid output.");

	if (iDepth > UI_MAX_HIERARCHY_DEPTH)
		return Fail(pError, strPath + L": Hierarchy is too deep.");

	if (!ElementJson.is_object())
		return Fail(pError, strPath + L": Element must be an object.");

	UI_ELEMENT_ASSET_DESC Element = {};

	Element.iElementID = ElementJson.value("ID", 0u);
	Element.strName = ElementJson.value("Name", string());
	Element.strBindingKey = ElementJson.value("BindingKey", string());

	const string strType = ElementJson.value("Type", string());

	if (!FromString(strType, &Element.eType))
		return Fail(pError, strPath + L": Invalid Element Type.");

	if (!ElementJson.contains("Transform") || !ElementJson["Transform"].is_object())
		return Fail(pError, strPath +  L": Transform block is missing.");

	if (ElementJson.contains("Transform"))
	{
		const json TransformJson = ElementJson["Transform"];

		UITransform Transform = {};

		if (!ReadFloat2(TransformJson.value("Position", json::array()), &Transform.vPosition))
			return Fail(pError, L"Invalid Transform.Position.");
		
		if (!ReadFloat2(TransformJson.value("Size", json::array()), &Transform.vSize))
			return Fail(pError, L"Invalid Transform.Size.");

		if (!ReadFloat2(TransformJson.value("Pivot", json::array()), &Transform.vPivot))
			return Fail(pError, L"Invalid Transform.Pivot.");

		const string strAnchor = TransformJson.value("Anchor", string());
		
		if (!FromString(strAnchor, &Transform.eAnchor))
			return Fail(pError, L"Invalid Transform.eAnchor.");
		
		Transform.fRotation = TransformJson.value("Rotation", 0.f);

		if (!ReadFloat2(TransformJson.value("Scale", json::array()), &Transform.vScale))
			return Fail(pError, L"Invalid Transform.Scale.");

		Element.Transform = Transform;
	}

	if (!ElementJson.contains("Render") || !ElementJson["Render"].is_object())
		return Fail(pError, strPath + L": Render block is missing.");

	if (ElementJson.contains("Render"))
	{
		const json RenderJson = ElementJson["Render"];

		UI_ASSET_RENDER_DESC Render = {};

		const string strRevealType = RenderJson.value("RevealType", string("None"));

		if (!FromString(strRevealType, &Render.eRevealType))
			return Fail(pError, L"Invalid Render.RevealType.");

		Render.fRevealProgress = RenderJson.value("RevealProgress", 1.f);
		Render.fRevealSoftness = RenderJson.value("RevealSoftness", 0.02f);

		if (RenderJson.contains("RevealCenter"))
		{
			if (!ReadFloat2(RenderJson["RevealCenter"], &Render.vRevealCenter))
				return Fail(pError, L"Invalid Render.RevealCenter.");
		}
		else
		{
			Render.vRevealCenter = _float2(0.5f, 0.5f);
		}

		if (!ReadFloat4(RenderJson.value("Color", json::array()), &Render.vColor))
			return Fail(pError, L"Invalid Render.Color.");
		
		if (!ReadFloat4(RenderJson.value("UVRect", json::array()), &Render.vUVRect))
			return Fail(pError, L"Invalid Render.UVRect.");
		
		Render.iSortOrder = RenderJson.value("SortOrder", 0u);
		Render.bVisible = RenderJson.value("Visible", true);
		Render.bUseTexture = RenderJson.value("UseTexture", true);

		Element.Render = Render;
	}

	if (Element.eType == UI_ELEMENT_TYPE::IMAGE)
	{
		if (!ElementJson.contains("Image") || !ElementJson["Image"].is_object())
			return Fail(pError, strPath + L": Image block is missing.");

		const json ImageJson = ElementJson["Image"];

		UI_IMAGE_ASSET_DESC Image = {};

		Image.strTexturePath = ImageJson.value("TexturePath", string());
		Image.strDissolveTexturePath = ImageJson.value("DissolveTexturePath", string());

		const string strDrawType = ImageJson.value("DrawType", string());
		if (!FromString(strDrawType, &Image.eDrawType))
			return Fail(pError, L"Invalid Image.DrawType.");

		if (!ReadFloat4(ImageJson.value("SliceMargin", json::array()), &Image.vSliceMargin))
			return Fail(pError, L"Invalid Image.SliceMargin.");

		if (!ReadFloat2(ImageJson.value("TextureSize", json::array()), &Image.vTextureSize))
			return Fail(pError, L"Invalid Image.TextureSize.");

		if (!ReadFloat2(ImageJson.value("SliceBaseSize", json::array()), &Image.vSliceBaseSize))
			return Fail(pError, L"Invalid Image.SliceBaseSize.");

		Element.Image = Image;
	}
	else if (Element.eType == UI_ELEMENT_TYPE::PROGRESS_BAR)
	{
		if (!ElementJson.contains("Progress") || !ElementJson["Progress"].is_object())
			return Fail(pError, strPath + L": Progress block is missing.");

		const json ProgressJson = ElementJson["Progress"];

		UI_PROGRESS_ASSET_DESC Progress = {};

		Progress.fLengthScale = ProgressJson.value("LengthScale", 1.f);

		const string strFillType = ProgressJson.value("FillType", string());
		if (!FromString(strFillType, &Progress.eFillType))
			return Fail(pError, L"Invalid Progress.FillType.");

		const string strFillDirection = ProgressJson.value("FillDirection", string());
		if (!FromString(strFillDirection, &Progress.eFillDirection))
			return Fail(pError, L"Invalid Progress.FillDirection.");

		const string strResizeDirection = ProgressJson.value("ResizeDirection", string());
		if (!FromString(strResizeDirection, &Progress.eResizeDirection))
			return Fail(pError, L"Invalid Progress.ResizeDirection.");

		if (!ProgressJson.contains("Slots") || !ProgressJson["Slots"].is_array())
			return Fail(pError, strPath + L": Progress.Slots must be an array.");
			
		for (const json& SlotJson : ProgressJson["Slots"])
		{
			if (!SlotJson.is_object())
				return Fail(pError, strPath + L": Progress slot must be an object.");

			UI_PROGRESS_SLOT_ASSET_DESC Slot = {};

			Slot.iSlotID = SlotJson.value("SlotID", -1);
			Slot.iTargetSlotID = SlotJson.value("TargetSlotID", -1);

			Slot.strName = SlotJson.value("Name", string());
			Slot.strBindingKey = SlotJson.value("BindingKey", string());
			Slot.strTexturePath = SlotJson.value("TexturePath", string());

			const string strSlotType = SlotJson.value("Type", string());
			if (!FromString(strSlotType, &Slot.eType))
				return Fail(pError, L"Invalid Slot.Type.");

			const string strSlotBlendMode = SlotJson.value("BlendMode", string());
			if (!FromString(strSlotBlendMode, &Slot.eBlendMode))
				return Fail(pError, L"Invalid Slot.BlendMode.");

			Slot.fAmount = SlotJson.value("Amount", 1.f);
			Slot.fStartAmount = SlotJson.value("StartAmount", 0.f);
			Slot.fEndAmount = SlotJson.value("EndAmount", 1.f);

			if (!ReadFloat4(SlotJson.value("UVRect", json::array()), &Slot.vUVRect))
				return Fail(pError, L"Invalid Slot.UVRect.");

			Slot.bStretchUVToRange = SlotJson.value("StretchUVToRange", false);

			if (!ReadFloat4(SlotJson.value("Color", json::array()), &Slot.vColor))
				return Fail(pError, L"Invalid Slot.Color.");

			if (!ReadFloat4(SlotJson.value("ColorLT", json::array()), &Slot.vColorLT))
				return Fail(pError, L"Invalid Slot.ColorLT.");

			if (!ReadFloat4(SlotJson.value("ColorRT", json::array()), &Slot.vColorRT))
				return Fail(pError, L"Invalid Slot.ColorRT.");

			if (!ReadFloat4(SlotJson.value("ColorRB", json::array()), &Slot.vColorRB))
				return Fail(pError, L"Invalid Slot.ColorRB.");

			if (!ReadFloat4(SlotJson.value("ColorLB", json::array()), &Slot.vColorLB))
				return Fail(pError, L"Invalid Slot.ColorLB.");

			Slot.iSortOffset = SlotJson.value("SortOffset", 0);
			Slot.bVisible = SlotJson.value("Visible", true);

			Progress.Slots.push_back(Slot);
		}

		Element.Progress = Progress;
	}
	else if (Element.eType == UI_ELEMENT_TYPE::TEXT)
	{
		if (!ElementJson.contains("Text") || !ElementJson["Text"].is_object())
		{
			return Fail(pError, strPath + L": Text block is missing.");
		}

		const json TextJson = ElementJson["Text"];

		UI_TEXT_ASSET_DESC Text = {};

		Text.strFontAssetID = TextJson.value("FontAssetID", string("DefaultUI"));
		Text.strPreviewText = TextJson.value("PreviewText", string("Text"));

		Text.fFontSize = TextJson.value("FontSize", 32.f);
		Text.fLineSpacing = TextJson.value("LineSpacing", 1.f);

		const string strHorizontalAlign = TextJson.value("HorizontalAlign", string("Left"));

		if (!FromString(strHorizontalAlign, &Text.eHorizontalAlign))
		{
			return Fail(pError, strPath + L": Invalid Text.HorizontalAlign.");
		}

		const string strVerticalAlign = TextJson.value("VerticalAlign", string("Top"));

		if (!FromString(strVerticalAlign, &Text.eVerticalAlign))
		{
			return Fail(pError, strPath + L": Invalid Text.VerticalAlign.");
		}

		Text.bWrap = TextJson.value("Wrap", false);

		Element.Text = move(Text);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::BUTTON)
	{
		if (!ElementJson.contains("Button") || !ElementJson["Button"].is_object())
		{
			return Fail(pError, strPath + L": Button block is missing.");
		}

		const json& ButtonJson = ElementJson["Button"];

		UI_BUTTON_ASSET_DESC Button = {};

		Button.strCommandKey = ButtonJson.value("CommandKey", string());
		Button.bInteractable = ButtonJson.value("Interactable", true);
		Button.bFocusOnHover = ButtonJson.value("FocusOnHover", false);
		Button.bSelectOnFocus = ButtonJson.value("SelectOnFocus", false);
		Button.bCommandOnPress = ButtonJson.value("CommandOnPress", false);
		Button.strFocusGroup = ButtonJson.value("FocusGroup", string("Default"));
		Button.iFocusOrder = ButtonJson.value("FocusOrder", 0);

		if (!ButtonJson.contains("States") || !ButtonJson["States"].is_object())
		{
			return Fail(pError, strPath + L": Button.States block is missing.");
		}

		const json& StatesJson = ButtonJson["States"];

		if (!StatesJson.contains("Hovered"))
		{
			return Fail(pError, strPath + L": Button.States.Hovered is missing.");
		}

		if (!StatesJson.contains("Pressed"))
		{
			return Fail(pError, strPath + L": Button.States.Pressed is missing.");
		}

		if (!StatesJson.contains("Selected"))
		{
			return Fail(pError, strPath + L": Button.States.Selected is missing.");
		}

		if (!StatesJson.contains("Focused"))
		{
			return Fail(pError, strPath + L": Button.States.Focused is missing.");
		}

		if (!StatesJson.contains("Disabled"))
		{
			return Fail(pError, strPath + L": Button.States.Disabled is missing.");
		}

		if (FAILED(ReadButtonVisualState(StatesJson["Hovered"], &Button.Hovered, pError, strPath + L".Button.States.Hovered")))
		{
			return E_FAIL;
		}

		if (FAILED(ReadButtonVisualState(StatesJson["Pressed"], &Button.Pressed, pError, strPath + L".Button.States.Pressed")))
		{
			return E_FAIL;
		}

		if (FAILED(ReadButtonVisualState(StatesJson["Selected"], &Button.Selected, pError, strPath + L".Button.States.Selected")))
		{
			return E_FAIL;
		}

		if (FAILED(ReadButtonVisualState(StatesJson["Focused"], &Button.Focused, pError, strPath + L".Button.States.Focused")))
		{
			return E_FAIL;
		}

		if (FAILED(ReadButtonVisualState(StatesJson["Disabled"], &Button.Disabled, pError, strPath + L".Button.States.Disabled")))
		{
			return E_FAIL;
		}

		Element.Button = move(Button);
	}
	else if (Element.eType == UI_ELEMENT_TYPE::COMPONENT_INSTANCE)
	{
		if (!ElementJson.contains("ComponentInstance") || !ElementJson["ComponentInstance"].is_object())
		{
			return Fail(pError, strPath + L": ComponentInstance block is missing.");
		}

		const json ComponentJson = ElementJson["ComponentInstance"];

		UI_COMPONENT_INSTANCE_ASSET_DESC ComponentInstance = {};

		ComponentInstance.strComponentAssetID = ComponentJson.value("AssetID", string());

		Element.ComponentInstance = move(ComponentInstance);
	}

	if (ElementJson.contains("Children"))
	{
		if (!ElementJson["Children"].is_array())
			return Fail(pError, strPath + L": Children must be an array");

		_uint iChildIndex = 0;

		for (const json& ChildJson : ElementJson["Children"])
		{
			UI_ELEMENT_ASSET_DESC Child = {};

			const _wstring strChildPath = strPath + L".Children[" + to_wstring(iChildIndex) + L"]";

			if (FAILED(Read_Element(ChildJson, &Child, pError, strChildPath, iDepth + 1)))
				return E_FAIL;

			Element.Children.push_back(move(Child));

			++iChildIndex;
		}
	}
	
	*pOutElement = move(Element);

	return S_OK;
}

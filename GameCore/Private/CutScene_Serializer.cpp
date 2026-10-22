#include "Engine_Utility.h"
#include "CutScene_Serializer.h"

static json CutScene_WriteFloat3(const _float3& Value)
{
	return json::array({ Value.x, Value.y, Value.z });
}

static json CutScene_WriteFloat4(const _float4& Value)
{
	return json::array({ Value.x, Value.y, Value.z, Value.w });
}

static _float3 CutScene_ReadFloat3(const json& Value, const _float3& DefaultValue)
{
	if (false == Value.is_array() || Value.size() < 3)
		return DefaultValue;

	return _float3(Value[0].get<_float>(), Value[1].get<_float>(), Value[2].get<_float>());
}

static _float4 CutScene_ReadFloat4(const json& Value, const _float4& DefaultValue)
{
	if (false == Value.is_array() || Value.size() < 4)
		return DefaultValue;

	return _float4(Value[0].get<_float>(), Value[1].get<_float>(), Value[2].get<_float>(), Value[3].get<_float>());
}

static json CutScene_WriteTransformValue(const GameCore::CUTSCENE_TRANSFORM_VALUE& Transform)
{
	json Value = {};
	Value["Scale"] = CutScene_WriteFloat3(Transform.vScale);
	Value["RotationDegree"] = CutScene_WriteFloat3(Transform.vRotationDegree);
	Value["Position"] = CutScene_WriteFloat3(Transform.vPosition);
	return Value;
}

static GameCore::CUTSCENE_TRANSFORM_VALUE CutScene_ReadTransformValue(const json& Value)
{
	GameCore::CUTSCENE_TRANSFORM_VALUE Transform = {};

	if (false == Value.is_object())
		return Transform;

	if (Value.contains("Scale"))
		Transform.vScale = CutScene_ReadFloat3(Value["Scale"], Transform.vScale);

	if (Value.contains("RotationDegree"))
		Transform.vRotationDegree = CutScene_ReadFloat3(Value["RotationDegree"], Transform.vRotationDegree);

	if (Value.contains("Position"))
		Transform.vPosition = CutScene_ReadFloat3(Value["Position"], Transform.vPosition);

	return Transform;
}

static json CutScene_WriteCameraTrack(const GameCore::CUTSCENE_CAMERA_TRACK& Track)
{
	json Value = {};
	Value["TrackID"] = Track.iTrackID;
	Value["TrackName"] = Track.strTrackName;
	Value["Keys"] = json::array();

	for (const GameCore::CUTSCENE_CAMERA_KEY& Key : Track.Keys)
	{
		Value["Keys"].push_back({
			{ "Time", Key.fTime },
			{ "Position", CutScene_WriteFloat3(Key.vPosition) },
			{ "Quaternion", CutScene_WriteFloat4(Key.vQuaternion) },
			{ "VerticalFovy", Key.fVerticalFovy }
		});
	}

	return Value;
}

static GameCore::CUTSCENE_CAMERA_TRACK CutScene_ReadCameraTrack(const json& Value)
{
	GameCore::CUTSCENE_CAMERA_TRACK Track = {};
	Track.iTrackID = Value.value("TrackID", UINT_MAX);
	Track.strTrackName = Value.value("TrackName", string{});

	if (Value.contains("Keys") && Value["Keys"].is_array())
	{
		for (const json& KeyValue : Value["Keys"])
		{
			GameCore::CUTSCENE_CAMERA_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.fVerticalFovy = KeyValue.value("VerticalFovy", XMConvertToRadians(60.f));

			if (KeyValue.contains("Position"))
				Key.vPosition = CutScene_ReadFloat3(KeyValue["Position"], Key.vPosition);

			if (KeyValue.contains("Quaternion"))
				Key.vQuaternion = CutScene_ReadFloat4(KeyValue["Quaternion"], Key.vQuaternion);

			Track.Keys.push_back(Key);
		}
	}

	return Track;
}

static json CutScene_WriteObjectTrack(const GameCore::CUTSCENE_OBJECT_TRACK& Track)
{
	json Value = {};
	Value["ObjectID"] = Track.iObjectID;
	Value["ObjectName"] = Engine::WStringToString(Track.strObjectName);
	Value["ObjectType"] = static_cast<_uint>(Track.eObjectType);
	Value["SpawnTime"] = Track.fSpawnTime;
	Value["DespawnTime"] = Track.fDespawnTime;
	Value["MeshName"] = Engine::WStringToString(Track.strMeshName);
	Value["MaterialName"] = Engine::WStringToString(Track.strMaterialName);
	Value["ModelResourceName"] = Engine::WStringToString(Track.strModelResourceName);
	Value["ModelPrototypeTag"] = Engine::WStringToString(Track.strModelPrototypeTag);
	Value["ModelLevel"] = Track.iModelLevel;
	Value["SkinnedModel"] = Track.bSkinnedModel;
	Value["AnimFolderTag"] = Engine::WStringToString(Track.strAnimFolderTag);
	Value["AnimTag"] = Engine::WStringToString(Track.strAnimTag);
	Value["RootBoneName"] = Track.strRootBoneName;
	Value["ParentObjectID"] = Track.iParentObjectID;
	Value["AttachBoneName"] = Track.strAttachBoneName;
	Value["ChildAttachBoneName"] = Track.strChildAttachBoneName;
	Value["InitialTransform"] = CutScene_WriteTransformValue(Track.InitialTransform);
	Value["InstanceSource"] = { { "MapName", Engine::WStringToString(Track.InstanceSource.strMapName) }, { "GroupIndex", Track.InstanceSource.iGroupIndex }, { "InstanceIndex", Track.InstanceSource.iInstanceIndex }, { "StaticObject", Track.InstanceSource.bStaticObject } };
	Value["TransformKeys"] = json::array();
	Value["EmissiveKeys"] = json::array();
	Value["DissolveKeys"] = json::array();
	Value["VisibilityKeys"] = json::array();
	Value["MeshVisibilityKeys"] = json::array();
	Value["AttachmentKeys"] = json::array();
	Value["AnimationSections"] = json::array();

	for (const GameCore::CUTSCENE_TRANSFORM_KEY& Key : Track.TransformKeys)
		Value["TransformKeys"].push_back({ { "Time", Key.fTime }, { "Transform", CutScene_WriteTransformValue(Key.Transform) } });

	for (const GameCore::CUTSCENE_FLOAT_KEY& Key : Track.EmissiveKeys)
		Value["EmissiveKeys"].push_back({ { "Time", Key.fTime }, { "Value", Key.fValue } });

	for (const GameCore::CUTSCENE_DISSOLVE_KEY& Key : Track.DissolveKeys)
		Value["DissolveKeys"].push_back({ { "Time", Key.fTime }, { "Amount", Key.fAmount }, { "Color", CutScene_WriteFloat4(Key.vColor) } });

	for (const GameCore::CUTSCENE_VISIBILITY_KEY& Key : Track.VisibilityKeys)
		Value["VisibilityKeys"].push_back({ { "Time", Key.fTime }, { "Visible", Key.bVisible } });

	for (const GameCore::CUTSCENE_MESH_VISIBILITY_KEY& Key : Track.MeshVisibilityKeys)
		Value["MeshVisibilityKeys"].push_back({ { "Time", Key.fTime }, { "MeshIndex", Key.iMeshIndex }, { "Visible", Key.bVisible } });

	for (const GameCore::CUTSCENE_ATTACHMENT_KEY& Key : Track.AttachmentKeys)
		Value["AttachmentKeys"].push_back({ { "Time", Key.fTime }, { "Attached", Key.bAttached }, { "ParentObjectID", Key.iParentObjectID }, { "AttachBoneName", Key.strAttachBoneName }, { "ChildAttachBoneName", Key.strChildAttachBoneName }, { "BlendTime", Key.fBlendTime }, { "InheritParentRotation", Key.bInheritParentRotation }, { "ParentRotationBlendTime", Key.fParentRotationBlendTime } });

	for (const GameCore::CUTSCENE_ANIMATION_SECTION& Section : Track.AnimationSections)
	{
		Value["AnimationSections"].push_back({ { "SectionID", Section.iSectionID }, { "StartTime", Section.fStartTime }, { "EndTime", Section.fEndTime }, { "ClipName", Engine::WStringToString(Section.strClipName) }, { "ClipStartTime", Section.fClipStartTime }, { "ClipEndTime", Section.fClipEndTime }, { "PlayRate", Section.fPlayRate }, { "BlendInTime", Section.fBlendInTime }, { "Loop", Section.bLoop }, { "UseRootMotion", Section.bUseRootMotion }, { "ResetRootMotion", Section.bResetRootMotion }, { "PositionCorrection", CutScene_WriteFloat3(Section.vPositionCorrection) }, { "RotationCorrectionDegree", CutScene_WriteFloat3(Section.vRotationCorrectionDegree) } });
	}

	return Value;
}

static GameCore::CUTSCENE_OBJECT_TRACK CutScene_ReadObjectTrack(const json& Value)
{
	GameCore::CUTSCENE_OBJECT_TRACK Track = {};
	Track.iObjectID = Value.value("ObjectID", UINT_MAX);
	Track.strObjectName = Engine::StringToWString(Value.value("ObjectName", string{}));
	Track.eObjectType = static_cast<GameCore::CUTSCENE_OBJECT_TYPE>(Value.value("ObjectType", ETOUI(GameCore::CUTSCENE_OBJECT_TYPE::END)));
	Track.fSpawnTime = Value.value("SpawnTime", 0.f);
	Track.fDespawnTime = Value.value("DespawnTime", FLT_MAX);
	Track.strMeshName = Engine::StringToWString(Value.value("MeshName", string{}));
	Track.strMaterialName = Engine::StringToWString(Value.value("MaterialName", string{}));
	Track.strModelResourceName = Engine::StringToWString(Value.value("ModelResourceName", string{}));
	Track.strModelPrototypeTag = Engine::StringToWString(Value.value("ModelPrototypeTag", string{}));
	Track.iModelLevel = Value.value("ModelLevel", 0u);
	// Version 5 이하의 기존 모델 Track은 모두 Skinned였으므로 true를 기본값으로 유지한다.
	Track.bSkinnedModel = Value.value("SkinnedModel", true);
	Track.strAnimFolderTag = Engine::StringToWString(Value.value("AnimFolderTag", string{}));
	Track.strAnimTag = Engine::StringToWString(Value.value("AnimTag", string{}));
	Track.strRootBoneName = Value.value("RootBoneName", string("Root"));
	Track.iParentObjectID = Value.value("ParentObjectID", UINT_MAX);
	Track.strAttachBoneName = Value.value("AttachBoneName", string{});
	Track.strChildAttachBoneName = Value.value("ChildAttachBoneName", string{});

	if (Value.contains("InitialTransform"))
		Track.InitialTransform = CutScene_ReadTransformValue(Value["InitialTransform"]);

	if (Value.contains("InstanceSource") && Value["InstanceSource"].is_object())
	{
		const json& Source = Value["InstanceSource"];
		Track.InstanceSource.strMapName = Engine::StringToWString(Source.value("MapName", string{}));
		Track.InstanceSource.iGroupIndex = Source.value("GroupIndex", UINT_MAX);
		Track.InstanceSource.iInstanceIndex = Source.value("InstanceIndex", UINT_MAX);
		Track.InstanceSource.bStaticObject = Source.value("StaticObject", false);
	}

	if (Value.contains("TransformKeys") && Value["TransformKeys"].is_array())
	{
		for (const json& KeyValue : Value["TransformKeys"])
		{
			GameCore::CUTSCENE_TRANSFORM_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);

			if (KeyValue.contains("Transform"))
				Key.Transform = CutScene_ReadTransformValue(KeyValue["Transform"]);

			Track.TransformKeys.push_back(Key);
		}
	}

	if (Value.contains("EmissiveKeys") && Value["EmissiveKeys"].is_array())
	{
		for (const json& KeyValue : Value["EmissiveKeys"])
		{
			GameCore::CUTSCENE_FLOAT_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.fValue = KeyValue.value("Value", 0.f);
			Track.EmissiveKeys.push_back(Key);
		}
	}

	if (Value.contains("DissolveKeys") && Value["DissolveKeys"].is_array())
	{
		for (const json& KeyValue : Value["DissolveKeys"])
		{
			GameCore::CUTSCENE_DISSOLVE_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.fAmount = KeyValue.value("Amount", 0.f);

			if (KeyValue.contains("Color"))
				Key.vColor = CutScene_ReadFloat4(KeyValue["Color"], Key.vColor);

			Track.DissolveKeys.push_back(Key);
		}
	}

	if (Value.contains("VisibilityKeys") && Value["VisibilityKeys"].is_array())
	{
		for (const json& KeyValue : Value["VisibilityKeys"])
		{
			GameCore::CUTSCENE_VISIBILITY_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.bVisible = KeyValue.value("Visible", true);
			Track.VisibilityKeys.push_back(Key);
		}
	}

	if (Value.contains("MeshVisibilityKeys") && Value["MeshVisibilityKeys"].is_array())
	{
		for (const json& KeyValue : Value["MeshVisibilityKeys"])
		{
			GameCore::CUTSCENE_MESH_VISIBILITY_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.iMeshIndex = KeyValue.value("MeshIndex", 0u);
			Key.bVisible = KeyValue.value("Visible", true);
			Track.MeshVisibilityKeys.push_back(Key);
		}
	}

	if (Value.contains("AttachmentKeys") && Value["AttachmentKeys"].is_array())
	{
		for (const json& KeyValue : Value["AttachmentKeys"])
		{
			GameCore::CUTSCENE_ATTACHMENT_KEY Key = {};
			Key.fTime = KeyValue.value("Time", 0.f);
			Key.bAttached = KeyValue.value("Attached", true);
			// Version 6 이하 파일은 Track 기본 부모/본을 그대로 사용해 기존 컷신을 유지한다.
			Key.iParentObjectID = KeyValue.value("ParentObjectID", Track.iParentObjectID);
			Key.strAttachBoneName = KeyValue.value("AttachBoneName", Track.strAttachBoneName);
			// Version 8 이하 파일은 자식 모델 원점을 사용하므로 빈 문자열을 기본값으로 유지한다.
			Key.strChildAttachBoneName = KeyValue.value("ChildAttachBoneName", Track.strChildAttachBoneName);
			// Version 7 이하 파일은 기존처럼 즉시 부착되도록 0초를 기본값으로 사용한다.
			Key.fBlendTime = max(0.f, KeyValue.value("BlendTime", 0.f));
			// Version 10 이하 파일은 기존처럼 부모 Bone의 회전을 그대로 상속한다.
			Key.bInheritParentRotation = KeyValue.value("InheritParentRotation", true);
			Key.fParentRotationBlendTime = max(0.f, KeyValue.value("ParentRotationBlendTime", 0.f));
			Track.AttachmentKeys.push_back(Key);
		}
	}

	if (Value.contains("AnimationSections") && Value["AnimationSections"].is_array())
	{
		for (const json& SectionValue : Value["AnimationSections"])
		{
			GameCore::CUTSCENE_ANIMATION_SECTION Section = {};
			Section.iSectionID = SectionValue.value("SectionID", UINT_MAX);
			Section.fStartTime = SectionValue.value("StartTime", 0.f);
			Section.fEndTime = SectionValue.value("EndTime", 0.f);
			Section.strClipName = Engine::StringToWString(SectionValue.value("ClipName", string{}));
			Section.fClipStartTime = SectionValue.value("ClipStartTime", 0.f);
			Section.fClipEndTime = SectionValue.value("ClipEndTime", 0.f);
			Section.fPlayRate = SectionValue.value("PlayRate", 1.f);
			// Version 13 이하 파일은 기존처럼 즉시 전환되도록 0초를 기본값으로 사용한다.
			Section.fBlendInTime = max(0.f, SectionValue.value("BlendInTime", 0.f));
			Section.bLoop = SectionValue.value("Loop", false);
			Section.bUseRootMotion = SectionValue.value("UseRootMotion", true);
			// Version 4 이하 파일은 false가 기본값이므로 기존 누적 방식이 그대로 유지된다.
			Section.bResetRootMotion = SectionValue.value("ResetRootMotion", false);
			// Version 9 이하 파일에는 Animation별 보정값이 없으므로 0을 기본값으로 유지한다.
			if (SectionValue.contains("PositionCorrection"))
				Section.vPositionCorrection = CutScene_ReadFloat3(SectionValue["PositionCorrection"], Section.vPositionCorrection);
			if (SectionValue.contains("RotationCorrectionDegree"))
				Section.vRotationCorrectionDegree = CutScene_ReadFloat3(SectionValue["RotationCorrectionDegree"], Section.vRotationCorrectionDegree);
			Track.AnimationSections.push_back(Section);
		}
	}

	return Track;
}

static json CutScene_WriteAsset(const GameCore::CUTSCENE_ASSET& Asset)
{
	json Root = {};
	// Animation Section별 위치/회전 보정값을 저장하는 파일은 최소 버전 10으로 기록한다.
	// 스태틱 맵 소스와 디졸브 키를 저장하는 파일은 최소 Version 16으로 기록한다.
	Root["Version"] = max(Asset.iVersion, 16u);
	Root["Name"] = Asset.strName;
	Root["Duration"] = Asset.fDuration;
	Root["FramesPerSecond"] = Asset.fFramesPerSecond;
	Root["CameraTracks"] = json::array();
	Root["CameraSceneSections"] = json::array();
	Root["CameraShakeEvents"] = json::array();
	Root["ScreenKeys"] = json::array();
	Root["ObjectTracks"] = json::array();
	Root["LightTracks"] = json::array();
	Root["SoundEvents"] = json::array();
	Root["EffectEvents"] = json::array();
	Root["NotifyBindings"] = json::array();

	for (const GameCore::CUTSCENE_CAMERA_TRACK& Track : Asset.CameraTracks)
		Root["CameraTracks"].push_back(CutScene_WriteCameraTrack(Track));

	for (const GameCore::CUTSCENE_CAMERA_SCENE_SECTION& Section : Asset.CameraSceneSections)
		Root["CameraSceneSections"].push_back({ { "SectionID", Section.iSectionID }, { "CameraTrackID", Section.iCameraTrackID }, { "StartTime", Section.fStartTime }, { "EndTime", Section.fEndTime }, { "SceneKey", Section.strSceneKey }, { "BlendIn", Section.fBlendIn }, { "BlendOut", Section.fBlendOut }, { "PlayRate", Section.fPlayRate }, { "PreviewPosition", CutScene_WriteFloat3(Section.vPreviewPosition) }, { "PreviewDirection", CutScene_WriteFloat3(Section.vPreviewDirection) }, { "ShowPreviewGuide", Section.bShowPreviewGuide } });

	for (const GameCore::CUTSCENE_CAMERA_SHAKE_EVENT& Event : Asset.CameraShakeEvents)
		Root["CameraShakeEvents"].push_back({ { "EventID", Event.iEventID }, { "Time", Event.fTime }, { "ShakeKey", Event.strShakeKey } });

	for (const GameCore::CUTSCENE_SCREEN_KEY& Key : Asset.ScreenKeys)
		Root["ScreenKeys"].push_back({ { "Time", Key.fTime }, { "FadeAmount", Key.fFadeAmount }, { "Color", CutScene_WriteFloat4(Key.vColor) }, { "ColorAmount", Key.fColorAmount } });

	for (const GameCore::CUTSCENE_OBJECT_TRACK& Track : Asset.ObjectTracks)
		Root["ObjectTracks"].push_back(CutScene_WriteObjectTrack(Track));

	for (const GameCore::CUTSCENE_LIGHT_TRACK& Track : Asset.LightTracks)
	{
		json TrackValue = { { "LightID", Track.iLightID }, { "SpawnForCutScene", Track.bSpawnForCutScene }, { "LightType", static_cast<_uint>(Track.eLightType) }, { "SpawnTime", Track.fSpawnTime }, { "DespawnTime", Track.fDespawnTime }, { "InitialTransform", CutScene_WriteTransformValue(Track.InitialTransform) }, { "Range", Track.fRange }, { "InnerDegree", Track.fInnerDegree }, { "OuterDegree", Track.fOuterDegree }, { "Shadow", Track.bShadow }, { "GodRay", Track.bGodRay }, { "GodRayIntensity", Track.fGodRayIntensity }, { "TransformKeys", json::array() }, { "Keys", json::array() } };

		for (const GameCore::CUTSCENE_TRANSFORM_KEY& TransformKey : Track.TransformKeys)
			TrackValue["TransformKeys"].push_back({ { "Time", TransformKey.fTime }, { "Transform", CutScene_WriteTransformValue(TransformKey.Transform) } });

		for (const GameCore::CUTSCENE_LIGHT_KEY& Key : Track.Keys)
			TrackValue["Keys"].push_back({ { "Time", Key.fTime }, { "Enable", Key.bEnable }, { "Intensity", Key.fIntensity }, { "Diffuse", CutScene_WriteFloat4(Key.vDiffuse) }, { "Ambient", CutScene_WriteFloat4(Key.vAmbient) }, { "Specular", CutScene_WriteFloat4(Key.vSpecular) } });

		Root["LightTracks"].push_back(TrackValue);
	}

	for (const GameCore::CUTSCENE_SOUND_EVENT& Event : Asset.SoundEvents)
		Root["SoundEvents"].push_back({ { "EventID", Event.iEventID }, { "StartTime", Event.fStartTime }, { "EndTime", Event.fEndTime }, { "SoundKey", Event.strSoundKey }, { "Channel", static_cast<_uint>(Event.eChannel) }, { "Volume", Event.fVolume }, { "Loop", Event.bLoop }, { "BGM", Event.bBGM }, { "UseWorldPosition", Event.bUseWorldPosition }, { "ObjectID", Event.iObjectID }, { "WorldPosition", CutScene_WriteFloat3(Event.vWorldPosition) } });

	for (const GameCore::CUTSCENE_EFFECT_EVENT& Event : Asset.EffectEvents)
		Root["EffectEvents"].push_back({ { "EventID", Event.iEventID }, { "StartTime", Event.fStartTime }, { "EndTime", Event.fEndTime }, { "EffectKey", Event.strEffectKey }, { "AttachType", static_cast<_uint>(Event.eAttachType) }, { "ObjectID", Event.iObjectID }, { "SecondaryObjectID", Event.iSecondaryObjectID }, { "BoneName", Event.strBoneName }, { "SecondaryBoneName", Event.strSecondaryBoneName }, { "PartObjectIDs", Event.PartObjectIDs }, { "WorldPosition", CutScene_WriteFloat3(Event.vWorldPosition) }, { "Offset", CutScene_WriteFloat3(Event.vOffset) }, { "Angle", CutScene_WriteFloat3(Event.vAngle) }, { "Loop", Event.bLoop } });

	for (const GameCore::CUTSCENE_NOTIFY_BINDING& Binding : Asset.NotifyBindings)
		Root["NotifyBindings"].push_back({ { "ObjectID", Binding.iObjectID }, { "NotifyHash", Binding.iNotifyHash }, { "ActionType", static_cast<_uint>(Binding.eActionType) }, { "ResourceKey", Binding.strResourceKey }, { "BoneName", Binding.strBoneName }, { "SoundChannel", static_cast<_uint>(Binding.eSoundChannel) }, { "Volume", Binding.fVolume }, { "Offset", CutScene_WriteFloat3(Binding.vOffset) } });

	return Root;
}

static _bool CutScene_HasValidLoadedEnums(const GameCore::CUTSCENE_ASSET& Asset)
{
	for (const GameCore::CUTSCENE_OBJECT_TRACK& Track : Asset.ObjectTracks)
	{
		if (ETOUI(Track.eObjectType) >= ETOUI(GameCore::CUTSCENE_OBJECT_TYPE::END))
			return false;
	}

	for (const GameCore::CUTSCENE_SOUND_EVENT& Event : Asset.SoundEvents)
	{
		if (Event.eChannel < SOUND_MASTER || Event.eChannel >= SOUND_END)
			return false;
	}

	for (const GameCore::CUTSCENE_LIGHT_TRACK& Track : Asset.LightTracks)
	{
		if (Track.bSpawnForCutScene && Track.eLightType >= LIGHT::END)
			return false;
	}

	for (const GameCore::CUTSCENE_EFFECT_EVENT& Event : Asset.EffectEvents)
	{
		if (ETOUI(Event.eAttachType) >= ETOUI(GameCore::CUTSCENE_EFFECT_ATTACH_TYPE::END))
			return false;
	}

	for (const GameCore::CUTSCENE_NOTIFY_BINDING& Binding : Asset.NotifyBindings)
	{
		if (ETOUI(Binding.eActionType) >= ETOUI(GameCore::CUTSCENE_NOTIFY_ACTION_TYPE::END) || Binding.eSoundChannel < SOUND_MASTER || Binding.eSoundChannel >= SOUND_END)
			return false;
	}

	return true;
}

HRESULT GameCore::CCutScene_Serializer::Save(const _wstring& strFilePath, const GameCore::CUTSCENE_ASSET& Asset, string* pErrorMessage)
{
	if (strFilePath.empty())
	{
		if (nullptr != pErrorMessage)
			*pErrorMessage = "Cutscene file path is empty.";
		return E_FAIL;
	}

	const json Root = CutScene_WriteAsset(Asset);

	ofstream File(strFilePath, ios::binary | ios::trunc);

	if (false == File.is_open())
	{
		if (nullptr != pErrorMessage)
			*pErrorMessage = "Failed to open cutscene file for writing.";
		return E_FAIL;
	}

	string strOutput = Root.dump(2);
	size_t iLinePosition = {};

	while (string::npos != (iLinePosition = strOutput.find('\n', iLinePosition)))
	{
		strOutput.insert(iLinePosition, 1, '\r');
		iLinePosition += 2;
	}

	// 컷신 데이터도 프로젝트 규칙에 맞춰 UTF-8 BOM/CRLF로 저장한다.
	const char UTF8BOM[] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
	File.write(UTF8BOM, sizeof(UTF8BOM));
	File.write(strOutput.data(), static_cast<streamsize>(strOutput.size()));

	if (false == File.good())
	{
		if (nullptr != pErrorMessage)
			*pErrorMessage = "Failed while writing cutscene file.";
		return E_FAIL;
	}

	return S_OK;
}

string GameCore::CCutScene_Serializer::Make_AssetSnapshot(const GameCore::CUTSCENE_ASSET& Asset)
{
	// 파일 I/O 없이 동일한 직렬화 형식으로 편집 전후 상태를 비교한다.
	return CutScene_WriteAsset(Asset).dump();
}

HRESULT GameCore::CCutScene_Serializer::Load(const _wstring& strFilePath, GameCore::CUTSCENE_ASSET* pOutAsset, string* pErrorMessage)
{
	if (strFilePath.empty() || nullptr == pOutAsset)
		return E_FAIL;

	ifstream File(strFilePath);

	if (false == File.is_open())
	{
		if (nullptr != pErrorMessage)
			*pErrorMessage = "Failed to open cutscene file.";
		return E_FAIL;
	}

	try
	{
		json Root = {};
		File >> Root;
		GameCore::CUTSCENE_ASSET Asset = {};
		Asset.iVersion = Root.value("Version", 1u);
		Asset.strName = Root.value("Name", string("NewCutScene"));
		Asset.fDuration = Root.value("Duration", 10.f);
		Asset.fFramesPerSecond = Root.value("FramesPerSecond", 60.f);

		if (Root.contains("CameraTracks") && Root["CameraTracks"].is_array())
		{
			for (const json& Value : Root["CameraTracks"])
				Asset.CameraTracks.push_back(CutScene_ReadCameraTrack(Value));
		}

		if (Root.contains("CameraSceneSections") && Root["CameraSceneSections"].is_array())
		{
			for (const json& Value : Root["CameraSceneSections"])
			{
				GameCore::CUTSCENE_CAMERA_SCENE_SECTION Section = {};
				Section.iSectionID = Value.value("SectionID", UINT_MAX);
				Section.iCameraTrackID = Value.value("CameraTrackID", UINT_MAX);
				Section.fStartTime = Value.value("StartTime", 0.f);
				Section.fEndTime = Value.value("EndTime", 0.f);
				Section.strSceneKey = Value.value("SceneKey", string{});
				Section.fBlendIn = Value.value("BlendIn", 0.2f);
				Section.fBlendOut = Value.value("BlendOut", 0.2f);
				Section.fPlayRate = Value.value("PlayRate", 1.f);
				Section.bShowPreviewGuide = Value.value("ShowPreviewGuide", true);

				if (Value.contains("PreviewPosition")) Section.vPreviewPosition = CutScene_ReadFloat3(Value["PreviewPosition"], Section.vPreviewPosition);
				if (Value.contains("PreviewDirection")) Section.vPreviewDirection = CutScene_ReadFloat3(Value["PreviewDirection"], Section.vPreviewDirection);
				Asset.CameraSceneSections.push_back(Section);
			}
		}

		// Version 2 파일은 SceneKey와 같은 이름의 Camera Track을 자동으로 연결한다.
		for (GameCore::CUTSCENE_CAMERA_SCENE_SECTION& Section : Asset.CameraSceneSections)
		{
			if (UINT_MAX != Section.iCameraTrackID)
				continue;

			for (const GameCore::CUTSCENE_CAMERA_TRACK& Track : Asset.CameraTracks)
			{
				if (Track.strTrackName == Section.strSceneKey)
				{
					Section.iCameraTrackID = Track.iTrackID;
					break;
				}
			}
		}

		if (Root.contains("CameraShakeEvents") && Root["CameraShakeEvents"].is_array())
		{
			for (const json& Value : Root["CameraShakeEvents"])
			{
				GameCore::CUTSCENE_CAMERA_SHAKE_EVENT Event = {};
				Event.iEventID = Value.value("EventID", UINT_MAX);
				Event.fTime = Value.value("Time", 0.f);
				Event.strShakeKey = Value.value("ShakeKey", string{});
				Asset.CameraShakeEvents.push_back(Event);
			}
		}

		if (Root.contains("ScreenKeys") && Root["ScreenKeys"].is_array())
		{
			for (const json& Value : Root["ScreenKeys"])
			{
				GameCore::CUTSCENE_SCREEN_KEY Key = {};
				Key.fTime = Value.value("Time", 0.f);
				Key.fFadeAmount = Value.value("FadeAmount", 0.f);
				Key.fColorAmount = Value.value("ColorAmount", 0.f);

				if (Value.contains("Color"))
					Key.vColor = CutScene_ReadFloat4(Value["Color"], Key.vColor);

				Asset.ScreenKeys.push_back(Key);
			}
		}

		if (Root.contains("ObjectTracks") && Root["ObjectTracks"].is_array())
		{
			for (const json& Value : Root["ObjectTracks"])
				Asset.ObjectTracks.push_back(CutScene_ReadObjectTrack(Value));
		}

		if (Root.contains("LightTracks") && Root["LightTracks"].is_array())
		{
			for (const json& Value : Root["LightTracks"])
			{
				GameCore::CUTSCENE_LIGHT_TRACK Track = {};
				Track.iLightID = Value.value("LightID", UINT_MAX);
				Track.bSpawnForCutScene = Value.value("SpawnForCutScene", false);
				Track.eLightType = static_cast<LIGHT>(Value.value("LightType", ETOUI(LIGHT::POINT)));
				Track.fSpawnTime = Value.value("SpawnTime", 0.f);
				Track.fDespawnTime = Value.value("DespawnTime", Asset.fDuration);
				Track.fRange = Value.value("Range", 10.f);
				Track.fInnerDegree = Value.value("InnerDegree", 20.f);
				Track.fOuterDegree = Value.value("OuterDegree", 30.f);
				Track.bShadow = Value.value("Shadow", true);
				Track.bGodRay = Value.value("GodRay", false);
				Track.fGodRayIntensity = Value.value("GodRayIntensity", 1.f);

				if (Value.contains("InitialTransform"))
					Track.InitialTransform = CutScene_ReadTransformValue(Value["InitialTransform"]);

				if (Value.contains("TransformKeys") && Value["TransformKeys"].is_array())
				{
					for (const json& TransformValue : Value["TransformKeys"])
					{
						GameCore::CUTSCENE_TRANSFORM_KEY TransformKey = {};
						TransformKey.fTime = TransformValue.value("Time", 0.f);

						if (TransformValue.contains("Transform"))
							TransformKey.Transform = CutScene_ReadTransformValue(TransformValue["Transform"]);

						Track.TransformKeys.push_back(TransformKey);
					}
				}

				if (Value.contains("Keys") && Value["Keys"].is_array())
				{
					for (const json& KeyValue : Value["Keys"])
					{
						GameCore::CUTSCENE_LIGHT_KEY Key = {};
						Key.fTime = KeyValue.value("Time", 0.f);
						Key.bEnable = KeyValue.value("Enable", true);
						Key.fIntensity = KeyValue.value("Intensity", 1.f);

						if (KeyValue.contains("Diffuse")) Key.vDiffuse = CutScene_ReadFloat4(KeyValue["Diffuse"], Key.vDiffuse);
						if (KeyValue.contains("Ambient")) Key.vAmbient = CutScene_ReadFloat4(KeyValue["Ambient"], Key.vAmbient);
						if (KeyValue.contains("Specular")) Key.vSpecular = CutScene_ReadFloat4(KeyValue["Specular"], Key.vSpecular);

						Track.Keys.push_back(Key);
					}
				}

				if (Track.Keys.empty())
				Track.Keys.push_back(GameCore::CUTSCENE_LIGHT_KEY{});

				Asset.LightTracks.push_back(Track);
			}
		}

		if (Root.contains("SoundEvents") && Root["SoundEvents"].is_array())
		{
			for (const json& Value : Root["SoundEvents"])
			{
				GameCore::CUTSCENE_SOUND_EVENT Event = {};
				Event.iEventID = Value.value("EventID", UINT_MAX);
				Event.fStartTime = Value.value("StartTime", 0.f);
				Event.fEndTime = Value.value("EndTime", 0.f);
				Event.strSoundKey = Value.value("SoundKey", string{});
				Event.eChannel = static_cast<CHANNEL_ID>(Value.value("Channel", static_cast<_uint>(SOUND_SFX)));
				Event.fVolume = Value.value("Volume", 1.f);
				Event.bLoop = Value.value("Loop", false);
				Event.bBGM = Value.value("BGM", false);
				Event.bUseWorldPosition = Value.value("UseWorldPosition", false);
				Event.iObjectID = Value.value("ObjectID", UINT_MAX);

				if (Value.contains("WorldPosition")) Event.vWorldPosition = CutScene_ReadFloat3(Value["WorldPosition"], Event.vWorldPosition);
				Asset.SoundEvents.push_back(Event);
			}
		}

		if (Root.contains("EffectEvents") && Root["EffectEvents"].is_array())
		{
			for (const json& Value : Root["EffectEvents"])
			{
				GameCore::CUTSCENE_EFFECT_EVENT Event = {};
				Event.iEventID = Value.value("EventID", UINT_MAX);
				Event.fStartTime = Value.value("StartTime", 0.f);
				Event.fEndTime = Value.value("EndTime", 0.f);
				Event.strEffectKey = Value.value("EffectKey", string{});
				Event.eAttachType = static_cast<GameCore::CUTSCENE_EFFECT_ATTACH_TYPE>(Value.value("AttachType", ETOUI(GameCore::CUTSCENE_EFFECT_ATTACH_TYPE::WORLD)));
				Event.iObjectID = Value.value("ObjectID", UINT_MAX);
				Event.iSecondaryObjectID = Value.value("SecondaryObjectID", UINT_MAX);
				Event.strBoneName = Value.value("BoneName", string{});
				Event.strSecondaryBoneName = Value.value("SecondaryBoneName", string{});
				Event.bLoop = Value.value("Loop", false);

				if (Value.contains("PartObjectIDs") && Value["PartObjectIDs"].is_array()) Event.PartObjectIDs = Value["PartObjectIDs"].get<vector<_uint>>();
				if (Value.contains("WorldPosition")) Event.vWorldPosition = CutScene_ReadFloat3(Value["WorldPosition"], Event.vWorldPosition);
				if (Value.contains("Offset")) Event.vOffset = CutScene_ReadFloat3(Value["Offset"], Event.vOffset);
				if (Value.contains("Angle")) Event.vAngle = CutScene_ReadFloat3(Value["Angle"], Event.vAngle);
				Asset.EffectEvents.push_back(Event);
			}
		}

		if (Root.contains("NotifyBindings") && Root["NotifyBindings"].is_array())
		{
			for (const json& Value : Root["NotifyBindings"])
			{
				GameCore::CUTSCENE_NOTIFY_BINDING Binding = {};
				Binding.iObjectID = Value.value("ObjectID", UINT_MAX);
				Binding.iNotifyHash = Value.value("NotifyHash", static_cast<_uint64>(0));
				Binding.eActionType = static_cast<GameCore::CUTSCENE_NOTIFY_ACTION_TYPE>(Value.value("ActionType", ETOUI(GameCore::CUTSCENE_NOTIFY_ACTION_TYPE::END)));
				Binding.strResourceKey = Value.value("ResourceKey", string{});
				Binding.strBoneName = Value.value("BoneName", string{});
				Binding.eSoundChannel = static_cast<CHANNEL_ID>(Value.value("SoundChannel", static_cast<_uint>(SOUND_SFX)));
				Binding.fVolume = Value.value("Volume", 1.f);

				if (Value.contains("Offset")) Binding.vOffset = CutScene_ReadFloat3(Value["Offset"], Binding.vOffset);
				Asset.NotifyBindings.push_back(Binding);
			}
		}

		if (false == CutScene_HasValidLoadedEnums(Asset))
		{
			if (nullptr != pErrorMessage)
				*pErrorMessage = "Cutscene file contains an invalid enum value.";

			return E_FAIL;
		}

		*pOutAsset = Asset;
		return S_OK;
	}
	catch (const json::exception& Exception)
	{
		if (nullptr != pErrorMessage)
			*pErrorMessage = Exception.what();
		return E_FAIL;
	}
}

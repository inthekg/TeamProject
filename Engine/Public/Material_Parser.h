#pragma once
#include "Engine_Defines.h"
#include "Material.h"

NS_BEGIN(Engine)

static SHADING_MODEL Parse_ShadingModel(const string& strName)
{
	if (strName.find("Subsurface") != std::string::npos) return SHADING_MODEL::SUBSURFACE;
	if (strName.find("Eye") != std::string::npos) return SHADING_MODEL::EYE;
	if (strName.find("Hair") != std::string::npos) return SHADING_MODEL::HAIR;
	//if (strName.find("FromMaterialExpression") != std::string::npos) return SHADING_MODEL::HAIR;
	if (strName.find("Unlit") != std::string::npos) return SHADING_MODEL::UNLIT;
	return SHADING_MODEL::DEFAULT_LIT;
}

static bool Is_HairLikeMaterial(const json& matJson)
{
	if (matJson.contains("Textures"))
	{
		for (auto& it : matJson["Textures"].items())
			if (it.key().find("BaseColor_Hair") != std::string::npos)
				return true;
	}

	if (matJson.contains("Parameters"))
	{
		const json& p = matJson["Parameters"];

		if (p.contains("Switches") && p["Switches"].contains("isHair_for_PC"))
			return true;

		if (p.contains("Colors")
			&& p["Colors"].contains("RootColor_Tint")
			&& p["Colors"].contains("TipColor_Tint"))
			return true;

		if (p.contains("Scalars"))
			for (auto& it : p["Scalars"].items())
				if (it.key().find("Fur_") != std::string::npos || it.key().find("Hair_") != std::string::npos)
					return true;
	}
	return false;
}

static bool Is_FurMaterial(const json& matJson)
{
	// 기본 Hair 텍스처 사용 여부가 아니라 실제 Fur 리소스 이름과 파라미터로 구분한다.
	if (matJson.contains("Textures") && matJson["Textures"].is_object())
	{
		for (auto& it : matJson["Textures"].items())
		{
			if (it.key().find("Fur") != string::npos)
				return true;

			if (it.value().is_string() && it.value().get<string>().find("Fur") != string::npos)
				return true;
		}
	}

	if (matJson.contains("Parameters") && matJson["Parameters"].contains("Scalars"))
	{
		for (auto& it : matJson["Parameters"]["Scalars"].items())
			if (0 == it.key().rfind("Fur_", 0))
				return true;
	}

	return false;
}

static bool Is_EyelashMaterial(const json& matJson)
{
	if (matJson.contains("Textures"))
	{
		for (auto& it : matJson["Textures"].items())
			if (it.key().find("Eyelash") != std::string::npos)
				return true;
	}

	if (matJson.contains("Parameters"))
	{
		const json& p = matJson["Parameters"];
		const char* secs[] = { "Colors", "Scalars", "Switches" };
		for (const char* s : secs)
		{
			if (p.contains(s))
				for (auto& it : p[s].items())
					if (it.key().find("Eyelash") != std::string::npos)
						return true;
		}
	}
	return false;
}

static bool Is_WaterMaterial(const json& matJson)
{
	if (!matJson.contains("Parameters"))
		return false;

	const json& p = matJson["Parameters"];
	const char* secs[] = { "Scalars", "Colors" };

	for (const char* s : secs)
	{
		if (!p.contains(s) || !p[s].is_object())
			continue;

		for (auto& it : p[s].items())
		{
			string k = it.key();
			transform(k.begin(), k.end(), k.begin(), ::tolower);

			if (0 == k.rfind("fluid_", 0))
				return true;
			if (0 == k.rfind("foam", 0))
				return true;
		}
	}
	return false;
}

static bool Is_MaskedWebMaterial(const json& matJson)
{
	if (!matJson.contains("Textures") || !matJson["Textures"].is_object())
		return false;

	for (auto& it : matJson["Textures"].items())
	{
		if (!it.value().is_string())
			continue;

		string v = it.value().get<string>();
		transform(v.begin(), v.end(), v.begin(), ::tolower);

		if (string::npos != v.find("_armk"))
			return true;
	}

	return false;
}

static BLEND_MODE ParseBlendMode(const json* bpo, int iTopLevelNum)
{
	if (bpo && bpo->contains("BlendMode"))
	{
		const string s = (*bpo)["BlendMode"].get<std::string>();
		if (s.find("Translucent") != std::string::npos) return BLEND_MODE::BLEND_TRANSLUCENT;
		if (s.find("Additive") != std::string::npos) return BLEND_MODE::BLEND_TRANSLUCENT;
		if (s.find("Masked") != std::string::npos) return BLEND_MODE::BLEND_MASKED;
		return BLEND_MODE::BLEND_OPAQUE;
	}
	switch (iTopLevelNum)
	{
	case 1:  return BLEND_MODE::BLEND_MASKED;
	case 2:  return BLEND_MODE::BLEND_TRANSLUCENT;
	default: return BLEND_MODE::BLEND_OPAQUE;
	}
}

static Material_Meta Build_MaterialMeta(const json& matJson)
{
	Material_Meta meta;
	if (!matJson.contains("Parameters")) return meta;

	const json& p = matJson["Parameters"];
	const int iBlendNum = p.value("BlendMode", 0);

	const json* bpo = nullptr;
	if (p.contains("Properties") && p["Properties"].contains("BasePropertyOverrides"))
		bpo = &p["Properties"]["BasePropertyOverrides"];

	const bool bHasSM = (bpo && bpo->contains("ShadingModel"));
	const string strSM = bHasSM ? (*bpo)["ShadingModel"].get<std::string>() : string();

	meta.eShadingModel = bHasSM ? Parse_ShadingModel(strSM) : SHADING_MODEL::DEFAULT_LIT;

	if (strSM.find("FromMaterialExpression") != std::string::npos)
		meta.eShadingModel = Is_HairLikeMaterial(matJson) ? SHADING_MODEL::HAIR : SHADING_MODEL::DEFAULT_LIT;

	if (meta.eShadingModel == SHADING_MODEL::HAIR && Is_EyelashMaterial(matJson))
		meta.eShadingModel = SHADING_MODEL::EYELASH;

	if (meta.eShadingModel == SHADING_MODEL::HAIR && Is_FurMaterial(matJson))
		meta.eShadingModel = SHADING_MODEL::FUR;

	meta.eBlendMode = ParseBlendMode(bpo, iBlendNum);

	if (meta.eBlendMode == BLEND_MODE::BLEND_TRANSLUCENT && Is_WaterMaterial(matJson))
		meta.eShadingModel = SHADING_MODEL::WATER;

	if (meta.eBlendMode == BLEND_MODE::BLEND_TRANSLUCENT && Is_MaskedWebMaterial(matJson))
		meta.eShadingModel = SHADING_MODEL::SPIDERWEB;


	if (bpo)
	{
		meta.bTwoSide = bpo->value("TwoSided", false);
		meta.fOpacityClip = bpo->value("OpacityMaskClipValue", 0.333f);
	}
	meta.bIsNull = p.value("IsNull", false);

	{
		const bool bHasEmissiveColor =
			p.contains("Colors") &&
			(p["Colors"].contains("Emissive_Factor") || p["Colors"].contains("EmissiveColor"));
		const bool bTextureless =
			(!matJson.contains("Textures")) || matJson["Textures"].empty();

		if (bHasEmissiveColor && bTextureless)
		{
			meta.eShadingModel = SHADING_MODEL::UNLIT;
			meta.bIsNull = false;
		}
	}

	return meta;
}

NS_END

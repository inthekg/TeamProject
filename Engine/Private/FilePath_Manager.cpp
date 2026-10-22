#include "FilePath_Manager.h"

void CFilePath_Manager::Set_ResourcePath(_wstring strPath)
{
	if(strPath.empty()) 	
	{
		MSG_BOX("CFilePath_Manager : Resource Path is empty!");
		return;
	}
	//(RESOURCE)/
	m_strResourcePath = strPath;

	//(RESOURCE)/Textures/
	m_strTextureBasePath = m_strResourcePath + TEXTURE_FOLDER;

	m_BasePaths[ETOUI(PATH_FILTER_TYPE::SKINNED_MODEL)]		= m_strResourcePath + TYPE_FOLDER[ETOUI(PATH_FILTER_TYPE::SKINNED_MODEL)];
	m_BasePaths[ETOUI(PATH_FILTER_TYPE::STATIC_MODEL)]		= m_strResourcePath + TYPE_FOLDER[ETOUI(PATH_FILTER_TYPE::STATIC_MODEL)];
	m_BasePaths[ETOUI(PATH_FILTER_TYPE::MAP)]				= m_strResourcePath + TYPE_FOLDER[ETOUI(PATH_FILTER_TYPE::MAP)];
	m_BasePaths[ETOUI(PATH_FILTER_TYPE::EFFECT)]			= m_strResourcePath + TYPE_FOLDER[ETOUI(PATH_FILTER_TYPE::EFFECT)];

}

_wstring CFilePath_Manager::Get_ResourcePath() const
{
	return m_strResourcePath;
}

/* ------------------------------------ Map Path ---------------------------------- */

// (ROUSOURDIR)/Maps/MapName/
_wstring CFilePath_Manager::Get_MapPath(const _wstring& strMapName) const
{
	return m_BasePaths[ETOUI(PATH_FILTER_TYPE::MAP)] + strMapName + L"/";
}

// (ROUSOURDIR)/Maps/MapName/MapName.data 
_wstring CFilePath_Manager::Get_MapMeshPath(const _wstring& strMapName) const
{
	return Get_MapPath(strMapName) + strMapName + MESH_FILE_TAG;
}


// (ROUSOURDIR)/Maps/MapName/Materials/strMapMaterialName.json 
_wstring CFilePath_Manager::Get_MapMaterialPath(const _wstring& strMapName, const _wstring& strMapMaterialName) const
{
	return Get_MapPath(strMapName) + MATERIAL_FOLDER + strMapMaterialName + MATERIAL_FILE_TAG;
}

/* -------------------------------- Static Model Path ------------------------------ */

// (ROUSOURDIR)/StaticModels/strModelName/
_wstring CFilePath_Manager::Get_StaticModelPath(const _wstring& strModelName) const
{
	return m_BasePaths[ETOUI(PATH_FILTER_TYPE::STATIC_MODEL)] + strModelName + L"/";
}
// (ROUSOURDIR)/StaticModels/strModelName/strModelName_Mesh.data
_wstring CFilePath_Manager::Get_StaticModelMeshPath(const _wstring& strModelName) const
{
	return Get_StaticModelPath(strModelName) + strModelName + MESH_FILE_TAG;
}

// (ROUSOURDIR)/StaticModels/strModelName/Materials/strMapMaterialName.json 
_wstring CFilePath_Manager::Get_StaticModelMaterialPath(const _wstring& strModelName, const _wstring& strModelMaterialName) const
{
	return Get_StaticModelPath(strModelName) + MATERIAL_FOLDER + strModelMaterialName + MATERIAL_FILE_TAG;
}

/* -------------------------------- Skinned Model ------------------------------ */

// (ROUSOURDIR)/SkinnedModels/strModelName/
_wstring CFilePath_Manager::Get_SkinnedModelPath(const _wstring& strModelName) const
{
	return m_BasePaths[ETOUI(PATH_FILTER_TYPE::SKINNED_MODEL)] + strModelName + L"/";
}

// (ROUSOURDIR)/SkinnedModels/strModelName/strModelName_Bone+Mesh.data
_wstring CFilePath_Manager::Get_SkinnedModelMeshPath(const _wstring& strModelName) const
{
	return Get_SkinnedModelPath(strModelName) + strModelName + BONEMESH_FILE_TAG;
}

// (ROUSOURDIR)/SkinnedModels/strModelName/Materials/strMapMaterialName.json 
_wstring CFilePath_Manager::Get_SkinnedModelMaterialPath(const _wstring& strModelName, const _wstring& strModelMaterialName) const
{
	return Get_SkinnedModelPath(strModelName) + MATERIAL_FOLDER + strModelMaterialName + MATERIAL_FILE_TAG;
}

// (ROUSOURDIR)/SkinnedModels/strModelName/strModelName_Animation.anim
_wstring CFilePath_Manager::Get_SkinnedAnimPath(const _wstring& strModelName) const
{
	return Get_SkinnedModelPath(strModelName) + strModelName + ANIM_FILE_TAG;
}

// (ROUSOURDIR)/SkinnedModels/strModelName/strModelName_AnimationData.csv
_wstring CFilePath_Manager::Get_SkinnedAnimCSVPath(const _wstring& strModelName) const
{
	return Get_SkinnedModelPath(strModelName) + strModelName + ANIM_CSV_FILE_TAG;
}

// (ROUSOURDIR)/Textures/strTextureName.dds
_wstring CFilePath_Manager::Get_TexturePath(const _wstring& strTextureName) const
{
	return m_strTextureBasePath + strTextureName + L".dds";
}

void CFilePath_Manager::Free()
{
}

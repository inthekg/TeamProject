#pragma once
#include "Base.h"
class CFilePath_Manager : public CBase
{
protected:
    CFilePath_Manager() {}
    ~CFilePath_Manager() =default;
    //TODO : 경로 확정되면 겜인턴에 Wrapping하기 !!!!!!!!!

public:
    void                            Set_ResourcePath(_wstring strPath);
	_wstring                        Get_ResourcePath() const;

    _wstring                        Get_MapPath(const _wstring& strMapName) const;
    _wstring                        Get_MapMeshPath(const _wstring& strMapName) const;
    _wstring                        Get_MapMaterialPath(const _wstring& strMapName, const _wstring& strMapMaterialName) const;

    _wstring                        Get_StaticModelPath(const _wstring& strModelName) const;
    _wstring                        Get_StaticModelMeshPath(const _wstring& strModelName) const;
    _wstring                        Get_StaticModelMaterialPath(const _wstring& strModelName, const _wstring& strModelMaterialName) const;

    _wstring                        Get_SkinnedModelPath(const _wstring& strModelName) const;
    _wstring                        Get_SkinnedModelMaterialPath(const _wstring& strModelName, const _wstring& strModelMaterialName) const;
    _wstring                        Get_SkinnedModelMeshPath(const _wstring& strModelName) const;
    _wstring                        Get_SkinnedAnimPath(const _wstring& strModelName) const;
    _wstring                        Get_SkinnedAnimCSVPath(const _wstring& strModelName) const;

    //const _wstring  Get_EffectPath(_wstring strModelName) const;

    _wstring                        Get_TexturePath(const _wstring& strTextureName) const;

private:
    //텍스쳐 폴더에서의 기본 경로 
   // _wstring                      m_TextureBasePaths[ETOUI(PATH_FILTER_TYPE::END)];

    //리소스 파일 기준 경로 (mesh파일있는곳)
    _wstring                        m_BasePaths[ETOUI(PATH_FILTER_TYPE::END)];

	_wstring                        m_strResourcePath                       = {};
    _wstring                        m_strTextureBasePath                    = {};

private:
    //파일 폴더 이름  
    //{ MAP, STATIC_MODEL, SKINNED_MODEL, EFFECT, END };
    static constexpr const wchar_t* TYPE_FOLDER[ETOUI(PATH_FILTER_TYPE::END)] =
    {
        L"/Maps/",
        L"/StaticModels/",
        L"/SkinnedModels/",
        L"/Effects/"
    };

    static constexpr const wchar_t*	MATERIAL_FOLDER         = { L"Materials/" };
    static constexpr const wchar_t* TEXTURE_FOLDER          = { L"/Textures/" };

    static constexpr const wchar_t* MATERIAL_FILE_TAG       = { L".json" };
    
    static constexpr const wchar_t* MESH_FILE_TAG           = { L"_Mesh.data" };
    static constexpr const wchar_t* BONEMESH_FILE_TAG       = { L"_Bone+Mesh.data" };
    static constexpr const wchar_t* PUREMESH_FILE_TAG       = { L"_PureMesh.data" };
    static constexpr const wchar_t* COOKEDMESH_FILE_TAG     = { L"_CookingMesh.data" };

    static constexpr const wchar_t* ANIM_FILE_TAG           = { L"_Animation.anim" };
    static constexpr const wchar_t* ANIM_CSV_FILE_TAG       = { L"_AnimationData.csv" };

public:
	static CFilePath_Manager*       Create();
	virtual void                    Free() override;

};


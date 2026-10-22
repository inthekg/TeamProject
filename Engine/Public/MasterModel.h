#pragma once
#include "Model.h"

NS_BEGIN(Engine)
class ENGINE_DLL CMasterModel : public CModel
{
private:
	CMasterModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMasterModel(const CMasterModel& Prototype);
	virtual ~CMasterModel() = default;

public:
	virtual HRESULT						Initialize_Prototype(_wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix);
	virtual HRESULT						Initialize(void* pArg);
private:
	virtual HRESULT						Ready_MeshBones(_wstring& strMeshBoneFilePath) override;

public:
	static CMasterModel*				Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix = XMMatrixIdentity());
	virtual CComponent*					Clone(void* pArg) override;
	virtual void						Free() override;
};

NS_END

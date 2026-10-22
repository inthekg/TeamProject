#pragma once
#include "Model.h"
NS_BEGIN(Engine)

class CMasterModel;

class ENGINE_DLL CPartsModel final : public CModel
{
public:
	typedef struct tagPartModelDesc
	{
		class CMasterModel* m_pMasterModel = {};
	} PARTMODEL_DESC;
private:
	CPartsModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPartsModel(const CPartsModel& Prototype);
	virtual ~CPartsModel() = default;

public:
	virtual HRESULT						Initialize_Prototype(_wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix);
	virtual HRESULT						Initialize(void* pArg);

private:
	virtual void						Update_MeshBones() override;
	virtual HRESULT						Ready_MeshBones(_wstring& strMeshBoneFilePath) override;

	HRESULT								Ready_MasterCombinedBoneMatrixBuffer();

private:
	class CMasterModel*					m_pMasterModel = {};
public:
	static CPartsModel*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strFolderName, _wstring strModelName, _fmatrix PreTransformMatrix = XMMatrixIdentity());
	//Clone할 때 같이 Clone한 MasterModel의 주소를 넣기
	virtual CComponent*					Clone(void* pArg) override;
	virtual void						Free() override;
};

NS_END

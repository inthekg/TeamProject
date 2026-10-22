#pragma once

#include "Model.h"
#include "Shader.h"
#include "Material.h"
#include "Mesh.h"
#include "VIBuffer_Rect.h"
#include "VIBuffer_Cube.h"
#include "VIBuffer_Point_Instance.h"
#include "VIBuffer_PXDebugLine.h"


NS_BEGIN(Engine)

class ENGINE_DLL CPrototype_Manager final : public CBase
{
private:
	CPrototype_Manager();
	virtual ~CPrototype_Manager() = default;

public:
	template <typename T>
	T*			Clone_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg);

public:
	HRESULT		Initialize(_uint iNumLevels);
	HRESULT		Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype);

	void		Clear(_uint iLevelIndex);

private:
	CBase*		Find_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag);


private:
	map<const _wstring, class CBase*>* m_pPrototypes = { nullptr };
	typedef map<const _wstring, class CBase*>	PROTOTYPES;

	size_t		m_iNumLevels = {};

public:
	static CPrototype_Manager* Create(_uint iNumLevels);
	virtual void Free() override;
};

template<typename T>
inline T* CPrototype_Manager::Clone_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
	CBase* pPrototype = Find_Prototype(iLevelIndex, strPrototypeTag);
	if (nullptr == pPrototype)
	{
		wstring errorLog = L"PrototypeMgr : Tag : " + strPrototypeTag + L" is not in Prototype Level " + std::to_wstring(iLevelIndex);
		MessageBox(NULL, errorLog.c_str(), L"System Message", MB_OK);
		return nullptr;
	}
	
	T* pInstance = dynamic_cast<T*>(pPrototype->Clone(pArg));
	if (nullptr == pInstance) return nullptr;
	
	return pInstance;
}

NS_END
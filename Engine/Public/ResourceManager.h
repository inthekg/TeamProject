#pragma once
#include "Base.h"
#include "Shader.h"
#include "VIBuffer.h"
#include "Texture.h"
#include "Material.h"
#include "Animation.h"
#include <shared_mutex>


//TODO : 씬전환시 Material 전체 초기화를 만약에 한다면 ID도 초기화하기

NS_BEGIN(Engine)

class CResource_Manager final : public CBase
{
private:
	CResource_Manager();
	virtual ~CResource_Manager() = default;


public:
	//런타임 해싱 함수
	HRESULT													Get_VIBuffer(_wstring strVIBufferName, CVIBuffer** pOutBuffer);
	HRESULT													Get_Texture(_wstring strTextureName, CTexture** pOutTexture);
	HRESULT													Get_Material(_wstring strMaterialName, CMaterial** pOutMaterial);
	HRESULT													Get_Shader(_wstring strShaderName, CShader** pOutShader);
	HRESULT													Get_Animation(_wstring strAnimationName, class CAnimation** pOutAnimation);

	//컴파일 타임 해싱 함수
	//constexpr 정수형에 문자열리터럴로 해싱한 키 값을 받고 매개변수로 전달하기 ex) constexpr _uint64 key = Hash_String("ResourceName");
	HRESULT													Get_VIBuffer(_uint64 iVIBufferKey, CVIBuffer** pOutBuffer);
	HRESULT													Get_Texture(_uint64 iTextureKey, CTexture** pOutTexture);
	HRESULT													Get_Material(_uint64 iMaterialKey, CMaterial** pOutMaterial);
	HRESULT													Get_Shader(_uint64 iShaderKey, CShader** pOutShader);
	HRESULT													Get_Animation(_uint64 iAnimationKey, class CAnimation** pOutAnimation);

public:
	//Material 관련 
	vector<CMaterial*>*										Get_MaterialVector() { return &m_MaterialVector; }
	_bool													Is_MaterialDirty() const { return m_bMaterialDirtyFlag; }
	void													Clear_MaterialDirtyFlag() { m_bMaterialDirtyFlag = false; }
	void													Get_MaterialSnapShot(vector<CMaterial*>& OutMaterials);

public:
	HRESULT													Input_VIBuffer(CVIBuffer* pVIBuffer);
	HRESULT													Input_Shader(CShader* pShader);
	HRESULT													Input_Texture(CTexture* pTexture);
	HRESULT													Input_Material(CMaterial* pMaterial, _wstring& KeyName);

	//Animation은 Name의 Hash값으로 관리함 
	//HRESULT 												Input_StaticAnimation(class CAnimation* pAnimation);
	//HRESULT 												Input_DynamicAnimation(class CAnimation* pAnimation);

	// 파일내 애니메이션 Hash 키값이 같은데 메쉬가 다르면  Bake 된 메쉬가 들어가져서 Mesh 가 찌그러짐
	HRESULT													Input_StaticAnimation(class CAnimation* pAnimation, _uint64 iKey = 0);
	HRESULT													Input_DynamicAnimation(class CAnimation* pAnimation, _uint64 iKey = 0);
	//HRESULT                                                 Input_PhysicsMaterial(PxMaterial* pPhysicsMaterial, _wstring& KeyName);


public:
	HRESULT													Input_TextureFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring strTextureFilePath);
	HRESULT													Input_MaterialFromFile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring MaterialFilePath);

	void													Release_DynamicAnimations();

private:

	CVIBuffer*												Find_VIBuffer(const _wstring& strVIBufferName);
	CShader*												Find_Shader(const _wstring& strShaderName);
	CTexture*												Find_Texture(const _wstring& strTextureName);
	CMaterial*												Find_Material(const _wstring& strMaterialName);
	CAnimation*												Find_Animation(const _wstring& strAnimationName);
	/*
	CAnimNotify*											Find_Notify(const _wstring& strNotifyName);
	PxMaterial*                                             Find_PhysicsMaterial(const _wstring& strPhysicsMaterialName);*/

	CVIBuffer*												Find_VIBuffer(_uint64 iVIBufferKey);
	CShader*												Find_Shader(_uint64 iShaderKey);
	CTexture*												Find_Texture(_uint64 iTextureKey);
	CMaterial*												Find_Material(_uint64 iMaterialKey);
	CAnimation*												Find_Animation(_uint64 iAnimationNameHashKey);



	void													Release_VIBuffer(_wstring& strVIBufferName);
	void													Release_Shader(_wstring& strShaderName);
	void													Release_Texture(_wstring& strTextureName);
	//void													Release_Material(_wstring& strMaterialName);
	void													Release_Animation(_wstring& strAnimationName);

	
private:
	//정점 버퍼 
	unordered_map<_uint64, CVIBuffer*>						m_VIBufferMap;

	//Shader 
	unordered_map<_uint64, CShader*>						m_ShaderMap;

	//텍스쳐
	unordered_map<_uint64, CTexture*>						m_TextureMap;

	//Material 
	unordered_map<_uint64, CMaterial*>						m_MaterialMap;
	//Buffer에 ID 순으로 바인딩하기 위한 컨테이너 
	vector<CMaterial*>										m_MaterialVector;



	//Static Animation - 리소스 매니저가 해제될때까지 유지되는 애니메이션
	unordered_map<_uint64, CAnimation*>						m_StaticAnimationMap;

	//Dynamic Animation - 레벨 전환시 해제되는 애니메이션 
	unordered_map<_uint64, CAnimation*>						m_DynamicAnimationMap;

	//   unordered_map<_uint64, PxMaterial*>					m_PhysicsMaterialMap;
	class CGameInstance*									m_pGameInstance = { nullptr };
private:
	//할당될 ID 
	_uint													m_iVIBufferID = { 1 };
	_uint													m_iShaderID = { 1 };
	_uint													m_iTextureID = { 1 };
	_uint													m_iMaterialID = { 1 };
	_uint 													m_iStaticAnimationID = { 1 };
	_uint 													m_iDynamicAnimationID = { 1 };
	
private:
	//const 함수에서도 사용가능하도록 mutable 
	mutable shared_mutex									m_ResourceLock;
	//버퍼 갱신이 필요한지 체크하는 플래그
	atomic<_bool>											m_bMaterialDirtyFlag = {};
public:
	static CResource_Manager* Create();
	virtual void											Free() override;
};

NS_END
#pragma once
#include "Engine_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CShader;

class ENGINE_DLL CShader_Resolver final  : public CBase
{
private :
	CShader_Resolver();
	virtual ~CShader_Resolver() = default;

public :
	// 셰이더 모두 생성된 뒤 1회 호출
	HRESULT		Initialize();

	// 머테이럴 메타 + (메쉬) 스키닝 -> 셰이더 , 폴백 보장
	CShader*	Resolve(const Material_Meta& tMeta, _bool bSkinned, _bool bInstanced = false) const;

	// 머테이럴 메타 + 랜더 그룹, IsNull -> End
	static RENDERID	Resolve_Group(const Material_Meta& tMeta);

	CShader*		Get_ReflectionShader(_bool bInstanced) const;

private :
	void		Register(Shader_Key tKey, CShader* pShader);
	CShader*	Find(SHADING_MODEL eShadingModel, BLEND_MODE eBlendMode, _bool bSkinned, _bool bInstanced) const;

private :
	class CGameInstance* m_pGameInstance = { nullptr };
	CShader* m_pReflectionStatic = { nullptr };
	CShader* m_pReflectionInstance = { nullptr };

	unordered_map<Shader_Key, CShader*, Shader_HashKey> m_ShaderTables;

public :
	static CShader_Resolver* Create();
	virtual void Free() override;
};

NS_END
#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

//TODO : 프레임워크 수정사항. Texture
//Texture 컴포넌트는 원본을 공유하는 방식으로 변경. 복사생성자 ,대입연산자는 삭제함
//또한 한 텍스쳐 클래스에는 하나의 SRV만 존재 

class ENGINE_DLL CTexture final : public CResource
{
private:
	CTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CTexture() = default;

public:
	ID3D11ShaderResourceView*	Get_SRV()
	{
		return m_pTexture;
	}

public:
	virtual HRESULT				Initialize_Prototype(_wstring* pTextureName, _wstring* pTextureFilePath);
	void						Bind_ShaderResource(_uint iSlot);

private:
	ID3D11ShaderResourceView*	m_pTexture = { nullptr };

public:
	static CTexture*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _wstring* pTextureName, _wstring* pTextureFilePath);
	virtual void				Free() override;
};

NS_END
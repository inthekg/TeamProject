#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

// 인스턴싱 정점 구조체는 pElements에 D3D11_INPUT_ELEMENT_DESC로 정의해서 넘겨주기
class ENGINE_DLL CShader final : public CResource
{
public:
	typedef struct tagShaderDesc
	{
		_wstring strShaderName = {};

		_wstring strVShaderFilePath = {};
		string strVShaderFuncName = {};

		_wstring strPShaderFilePath = {};
		string strPShaderFuncName = {};
		
		_wstring strGShaderFilePath = {};
		string strGShaderFuncName = {};
		
		D3D11_INPUT_ELEMENT_DESC* pElements = nullptr;
		_uint iNumElements = 0;
	} SHADER_DESC;

private:
	CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CShader() = default;

public:
	virtual HRESULT			Initialize_Prototype(SHADER_DESC& shaderDesc);
	void					Render();

public:
	ID3D11VertexShader*		Get_VertexShader() { return m_pVertexShader; }
	ID3D11PixelShader*		Get_PixelShader() { return m_pPixelShader; }
	ID3D11GeometryShader*	Get_GeometryShader() { return m_pGeometryShader; }
	//추가 셰이더 필요하면 추가 
	ID3D11InputLayout*		Get_InputLayout() { return m_pInputLayout;  };

	////Material Buffer 리플렉션 정보 관련 
	_uint					Get_MaterialBufferSize() const { return m_iMaterialBufferSize; }
	const unordered_map<string, Shader_ReflectionData>& Get_ReflectionMap() const { return m_MaterialBuffer_Reflection; }
	_bool					Has_MaterialBuffer() const { return m_iMaterialBufferSize > 0; }

private:
	HRESULT					Create_VertexShader(string& VSName, const _uint& iHlslFlag);
	HRESULT					Create_PixelShader(string& PSName, const _uint& iHlslFlag);
	HRESULT					Create_GeometryShader(string& GSName, const _uint& iHlslFlag);

	HRESULT					Create_InputLayout();
	HRESULT					Create_InputLayout(const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements);

	HRESULT					Create_MaterialBuffer_Reflection();


private:
	//컴파일 된 바이너리 데이터. VS만 inputLayout 생성할 때 필요하므로 저장함.
	ID3D10Blob*				m_pVertexBlob = { nullptr };
	ID3D10Blob*				m_pPixelBlob = { nullptr };

private:
	ID3D11VertexShader*		m_pVertexShader = { nullptr };
	ID3D11PixelShader*		m_pPixelShader = { nullptr };
	ID3D11GeometryShader*	m_pGeometryShader = { nullptr };
	ID3D11InputLayout*		m_pInputLayout = { nullptr };

	//셰이더 메테리얼 인스턴스 버퍼 리플렉션 
	//string주의 . 셰이더 변수명과 JSON을 고려하여 string으로 함 
	unordered_map<string, Shader_ReflectionData> m_MaterialBuffer_Reflection;
	_uint					m_iMaterialBufferSize = {};

	wstring					m_strVSShaderFilePath = {};
	wstring					m_strPSShaderFilePath = {};
	wstring					m_strGSShaderFilePath = {};

	_bool					m_bFlagCS = {};
public:
	static CShader*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, SHADER_DESC& shaderDesc);
	virtual void			Free() override;
};

NS_END
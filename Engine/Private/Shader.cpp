#include "Shader.h"

CShader::CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CResource{ pDevice, pContext }
{
}

HRESULT CShader::Initialize_Prototype(SHADER_DESC& shaderDesc)
{
    if (shaderDesc.strShaderName.empty())
    {
        MSG_BOX("Shader Create Fail : Shader Name Empty");
        return E_FAIL;
    }

    Set_Name(shaderDesc.strShaderName);

    
    _uint       iHlslFlag = {};
#ifdef _DEBUG
   // iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS;
    iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
    iHlslFlag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif

    if(shaderDesc.strVShaderFilePath.empty())
    {
        MSG_BOX("Shader Create Fail : Vertex Shader Path Empty ");
        return E_FAIL;
	}

    //VSShader Create
	m_strVSShaderFilePath = shaderDesc.strVShaderFilePath;
    if (FAILED(Create_VertexShader(shaderDesc.strVShaderFuncName, iHlslFlag)))
    {
        MSG_BOX("Shader Create Fail : Vertex Shader Compile Failed");
        return E_FAIL;
    }

    //PS 부터는 경로를 별도로 지정하지 않으면 VS경로 사용 
    //VS, PS는 반드시 생성되어야 함 
    
    //PSShader Create
    if (shaderDesc.strPShaderFilePath.empty())  m_strPSShaderFilePath = m_strVSShaderFilePath;
	else m_strPSShaderFilePath = shaderDesc.strPShaderFilePath;
    
    if (FAILED(Create_PixelShader(shaderDesc.strPShaderFuncName, iHlslFlag)))
    {
        MSG_BOX("Shader Create Fail : Pixel Shader Compile Failed");
        return E_FAIL;
    }


	//이름이 없으면 GS는 사용 안하는 것으로 간주
    //GSShader Create
    if (shaderDesc.strGShaderFuncName.empty() == false)
    {
        if (shaderDesc.strGShaderFilePath.empty())  m_strGSShaderFilePath = m_strVSShaderFilePath;
        else m_strGSShaderFilePath = shaderDesc.strGShaderFilePath;

        if (FAILED(Create_GeometryShader(shaderDesc.strGShaderFuncName, iHlslFlag)))
        {
            MSG_BOX("Shader Create Fail : Geometry Shader Compile Failed");
            return E_FAIL;
        }
    }


    if (shaderDesc.pElements == nullptr)
    {
        if (FAILED(Create_InputLayout())) return E_FAIL;
    }
    else
    {
		if (FAILED(Create_InputLayout(shaderDesc.pElements, shaderDesc.iNumElements))) return E_FAIL;
    }

    if(FAILED(Create_MaterialBuffer_Reflection()))
    {
        MSG_BOX("Shader Create Fail : Material Buffer Reflection Failed");
        return E_FAIL;
	}   

    Safe_Release(m_pVertexBlob);
    Safe_Release(m_pPixelBlob);

    return S_OK;
}

void CShader::Render()
{
    if (m_pVertexShader != nullptr) m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
    if (m_pPixelShader != nullptr) m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);
    if (m_pInputLayout != nullptr) m_pContext->IASetInputLayout(m_pInputLayout);
}


HRESULT CShader::Create_VertexShader(string& VSName, const _uint& iHlslFlag)
{
    ID3D10Blob* ErrorBlob = nullptr;
    if (FAILED(D3DCompileFromFile(m_strVSShaderFilePath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, VSName.c_str(), "vs_5_0", iHlslFlag, 0, &m_pVertexBlob, &ErrorBlob)))
    {
        if (ErrorBlob)
            MessageBoxA(nullptr, (char*)ErrorBlob->GetBufferPointer(), "Shader Compile Error", MB_OK);
        else
            MessageBoxA(nullptr, "Unknown error or file not found.", "Shader Compile Error", MB_OK);

        Safe_Release(ErrorBlob);
        return E_FAIL;
    }

    if (FAILED(m_pDevice->CreateVertexShader(m_pVertexBlob->GetBufferPointer(), m_pVertexBlob->GetBufferSize(), nullptr, &m_pVertexShader)))
    {
        MSG_BOX("Failed to VertexShader Create");
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CShader::Create_PixelShader(string& PSName, const _uint& iHlslFlag)
{
    ID3D10Blob* ErrorBlob = { nullptr };
    if (FAILED(D3DCompileFromFile(m_strPSShaderFilePath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, PSName.c_str(), "ps_5_0", iHlslFlag, 0, &m_pPixelBlob, &ErrorBlob)))
    {
        if (ErrorBlob)
            MessageBoxA(nullptr, (char*)ErrorBlob->GetBufferPointer(), "Shader Compile Error", MB_OK);
        else 
            MessageBoxA(nullptr, "Unknown error or file not found.", "Shader Compile Error", MB_OK);

        Safe_Release(ErrorBlob);
        return E_FAIL;
    }

    if (FAILED(m_pDevice->CreatePixelShader(m_pPixelBlob->GetBufferPointer(), m_pPixelBlob->GetBufferSize(), nullptr, &m_pPixelShader)))
    {
        MSG_BOX("Failed to PixelShader Create");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CShader::Create_GeometryShader(string& GSName, const _uint& iHlslFlag)
{
    ID3D10Blob* ErrorBlob = { nullptr };
    ID3D10Blob* pGeometryBlob = { nullptr };
    if (FAILED(D3DCompileFromFile(m_strGSShaderFilePath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, GSName.c_str(), "gs_5_0", iHlslFlag, 0, &pGeometryBlob, &ErrorBlob)))
    {
        if (ErrorBlob)
            MessageBoxA(nullptr, (char*)ErrorBlob->GetBufferPointer(), "Shader Compile Error", MB_OK);
        else
            MessageBoxA(nullptr, "Unknown error or file not found.", "Shader Compile Error", MB_OK);

        Safe_Release(ErrorBlob);
        return E_FAIL;
    }

    if (FAILED(m_pDevice->CreateGeometryShader(pGeometryBlob->GetBufferPointer(), pGeometryBlob->GetBufferSize(), nullptr, &m_pGeometryShader)))
        return E_FAIL;

    Safe_Release(pGeometryBlob);
    return S_OK;
}

HRESULT CShader::Create_InputLayout()
{
    if (m_pVertexBlob == nullptr) return E_FAIL;
    vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescs;

    ID3D11ShaderReflection* pReflection = { nullptr };
    if (FAILED(D3DReflect(
        m_pVertexBlob->GetBufferPointer(), 
        m_pVertexBlob->GetBufferSize(), 
        IID_ID3D11ShaderReflection, 
        reinterpret_cast<void**>(&pReflection)))
        ) return E_FAIL;

    D3D11_SHADER_DESC shaderDesc{};
    pReflection->GetDesc(&shaderDesc);

    for (_uint i = 0; i < shaderDesc.InputParameters; i++)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        pReflection->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC elementDesc{};
        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = 0;
        //자동으로 Offset 계산
        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        //Mask는 데이터의 성분에 대한 비트 마스크 , ComponentType은 어떤 자료형인지 
        //32비트 float, uint만 처리함. 다른 형식 필요하다면 추가하기 
        //D3D_Component_mask_w 이 8 인것으로 보아 비트는 wzyx = abgr 순
        if (paramDesc.Mask == 1) // R = 0001 
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
        }
        else if (paramDesc.Mask <= 3) // RG = 0011
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
        }
        else if (paramDesc.Mask <= 7) // RGB = 0111
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
        }
        else if (paramDesc.Mask <= 15) // RGBA = 1111
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
        }

        inputElementDescs.push_back(elementDesc);
    }

    HRESULT hr = m_pDevice->CreateInputLayout(&inputElementDescs[0], (_uint)inputElementDescs.size(), m_pVertexBlob->GetBufferPointer(), m_pVertexBlob->GetBufferSize(), &m_pInputLayout);

    Safe_Release(pReflection);
    return hr;
}

HRESULT CShader::Create_InputLayout(const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements)
{
    if (m_pVertexBlob == nullptr || pElements == nullptr || iNumElements == 0) return E_FAIL;

    return m_pDevice->CreateInputLayout(pElements, iNumElements, m_pVertexBlob->GetBufferPointer(), m_pVertexBlob->GetBufferSize(), &m_pInputLayout);
}
HRESULT CShader::Create_MaterialBuffer_Reflection()
{
    if (m_pPixelBlob == nullptr) return E_FAIL;

    ID3D11ShaderReflection* pReflection = { nullptr };

    if (FAILED(D3DReflect(
                m_pPixelBlob->GetBufferPointer(),
                m_pPixelBlob->GetBufferSize(),
                IID_ID3D11ShaderReflection,
                reinterpret_cast<void**>(&pReflection)))
                ) return E_FAIL;

    //PS의 메테리얼 상수버퍼 가져옴 
    ID3D11ShaderReflectionConstantBuffer* pCB = pReflection->GetConstantBufferByName("PerMaterialBuffer");
    D3D11_SHADER_BUFFER_DESC cbDesc;
    ZeroMemory(&cbDesc, sizeof(cbDesc));

    if (pCB == nullptr || FAILED(pCB->GetDesc(&cbDesc)))
    {
        m_iMaterialBufferSize = 0;          // 이 쉐이더에는 b3 데이터 없음
        Safe_Release(pReflection);
        return S_OK;
    }

    m_iMaterialBufferSize = cbDesc.Size;

    for (_uint i = 0; i < cbDesc.Variables; ++i)
    {
        ID3D11ShaderReflectionVariable* pVar = pCB->GetVariableByIndex(i);
        D3D11_SHADER_VARIABLE_DESC varDesc{};
        if (FAILED(pVar->GetDesc(&varDesc))) continue;

        ID3D11ShaderReflectionType* pType = pVar->GetType();
        D3D11_SHADER_TYPE_DESC typeDesc{};
        ZeroMemory(&typeDesc, sizeof(typeDesc));
        if(FAILED(pType->GetDesc(&typeDesc))) continue;

        Shader_ReflectionData reflectionData;
        reflectionData.iOffset = varDesc.StartOffset;
        reflectionData.eValueType = SHADER_VALUE_TYPE::END;

        if (typeDesc.Class == D3D_SVC_SCALAR)
            reflectionData.eValueType = SHADER_VALUE_TYPE::SCALAR;
        else if (typeDesc.Class == D3D_SVC_VECTOR)
            reflectionData.eValueType = SHADER_VALUE_TYPE::COLOR;

        if (reflectionData.eValueType != SHADER_VALUE_TYPE::END)
            m_MaterialBuffer_Reflection[varDesc.Name] = reflectionData;
    }

    Safe_Release(pReflection);
    return S_OK;
}


//
//HRESULT CShader::Create_MaterialBuffer_Reflection()
//{
//    if (m_pPixelBlob == nullptr) return E_FAIL;
//
//    ID3D11ShaderReflection* pReflection = { nullptr };
//
//    if (FAILED(D3DReflect(
//        m_pPixelBlob->GetBufferPointer(),
//        m_pPixelBlob->GetBufferSize(),
//        IID_ID3D11ShaderReflection,
//        reinterpret_cast<void**>(&pReflection)))
//        ) return E_FAIL;
//
//    //PS의 메테리얼 상수버퍼 가져옴 
//    ID3D11ShaderReflectionConstantBuffer* pCB = pReflection->GetConstantBufferByName("PerMaterialBuffer");
//    D3D11_SHADER_BUFFER_DESC cbDesc;
//    ZeroMemory(&cbDesc, sizeof(cbDesc));
//    pCB->GetDesc(&cbDesc);
//
//    m_iMaterialBufferSize = cbDesc.Size;
//    for (UINT i = 0; i < cbDesc.Variables; i++)
//    {
//        ID3D11ShaderReflectionVariable* pVar = pCB->GetVariableByIndex(i);
//        D3D11_SHADER_VARIABLE_DESC varDesc;
//        if (FAILED(pVar->GetDesc(&varDesc))) continue;
//
//        ID3D11ShaderReflectionType* pType = pVar->GetType();
//        D3D11_SHADER_TYPE_DESC typeDesc;
//        ZeroMemory(&typeDesc, sizeof(typeDesc));
//        if (FAILED(pType->GetDesc(&typeDesc))) continue;
//
//        Shader_ReflectionData reflectionData;
//        reflectionData.iOffset = varDesc.StartOffset;
//        reflectionData.eValueType = SHADER_VALUE_TYPE::END;
//
//        //자료형이 Flaot이면
//        if (typeDesc.Type == D3D_SVC_SCALAR)
//        {
//            //형태가 스칼라 하나면
//            if (typeDesc.Class == D3D_SVC_SCALAR)
//            {
//                reflectionData.eValueType = SHADER_VALUE_TYPE::SCALAR;
//            }
//            //벡터면 
//            else if (typeDesc.Class == D3D_SVC_VECTOR)
//            {
//                reflectionData.eValueType = SHADER_VALUE_TYPE::COLOR;
//            }
//        }
//
//        // 변수 이름과 리플렉션 정보를 맵에 저장
//        m_MaterialBuffer_Reflection[varDesc.Name] = reflectionData;
//    }
//
//    Safe_Release(pReflection);
//    return S_OK;
//}

CShader* CShader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, SHADER_DESC& shaderDesc)
{
    CShader* pInstance = new CShader(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(shaderDesc)))
    {
        MSG_BOX("Failed to Created : CShader");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CShader::Free()
{
    __super::Free();
    Safe_Release(m_pInputLayout);
    Safe_Release(m_pVertexShader);
    Safe_Release(m_pPixelShader);
    Safe_Release(m_pGeometryShader);

}

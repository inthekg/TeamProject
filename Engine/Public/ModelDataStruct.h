#pragma once 

#include "Engine_Defines.h"

namespace Engine
{
	struct BoneData
	{
		_char                   boneName[MAX_NAME_LEN];
		int32_t                 parentIndex;
		_float4x4               transformMatrix;
	};

	struct MeshData
	{
		_char                   meshName[MAX_NAME_LEN];
		_char                   materialName[MAX_NAME_LEN];

		_bool                   HasAnimation;
		_uint					vertexCount;
		_uint					indexCount;
		_uint					vertexStride;
		_uint					boneCount;
	};

	//TODO : 0607 맵 인스턴싱 추가 ksb
	struct MapInstanceData
	{
		_char instanceName[MAX_NAME_LEN]; 
		_float4x4				worldMatrix;
	};


	struct MeshPassData
	{
		_char                   name[MAX_NAME_LEN];
		_char                   materialName[MAX_NAME_LEN];

		_bool                   hasAnimation;
		_uint	                vertexCount;
		_uint	                indexCount;
		_uint	                vertexStride;
		_uint	                boneCount;

		vector<VTXANIMMESH>     AnimVertices;
		vector<VTXMESH>			NoAnimVertices;
        vector<VTXPOS>          PureVertices;
		vector<_uint>			Indices;
		vector<_uint>			BoneIndices;
		vector<_float4x4>       OffsetMatrices;
	};

	struct AnimationReadData
	{
        _wstring				strAnimationName;
		_wstring				strAnimationFileName;
		_float                  fDuration;
		_float                  fTickPerSecond;
		_bool                   bLoop;
		_float                  fBlendTime;
		_bool                   bRootMotion;
	};

	struct ChannelData
	{
		// 추가
		_char					szBoneName[MAX_NAME_LEN];
		_uint					iBoneIndex;
		_uint					iTranslationKeyFrameCount;
		_uint					iScaleKeyFrameCount;
		_uint					iRotationKeyFrameCount;
	};

	struct ChannelPassData
	{
		_uint                   iBoneIndex;
		_uint                   iTranslationKeyFrameCount;
		_uint                   iScaleKeyFrameCount;
		_uint                   iRotationKeyFrameCount;

		vector<KEYFRAME_VEC>    TransKeyFrames;
		vector<KEYFRAME_VEC>    ScaleKeyFrames;
		vector<KEYFRAME_QUAT>   RotKeyFrames;
	};





};


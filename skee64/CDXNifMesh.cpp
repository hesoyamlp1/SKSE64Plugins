#include "CDXNifMesh.h"
#include "CDXNifMaterial.h"
#include "CDXScene.h"
#include "CDXShader.h"

#include "RE/N/NiGeometry.h"
#include "RE/N/NiRTTI.h"
#include "RE/N/NiExtraData.h"

#include "NifUtils.h"

#include <thread>
#include <mutex>
#include <vector>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "half.hpp"

#include "REX/W32/D3D11_3.h"
#include <cstdint>



using namespace DirectX;

CDXNifMesh::CDXNifMesh() : CDXEditableMesh()
{
	m_material = nullptr;
	m_morphable = false;
}

CDXNifMesh::~CDXNifMesh()
{
	
}

CDXMeshVert * CDXNifMesh::LockVertices(const LockMode type)
{
	EnterCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);
	return CDXMesh::LockVertices(type);
}

CDXMeshIndex * CDXNifMesh::LockIndices()
{
	EnterCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);
	return CDXMesh::LockIndices();
}

void CDXNifMesh::UnlockVertices(const LockMode type)
{
	CDXMesh::UnlockVertices(type);
	LeaveCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);
}
void CDXNifMesh::UnlockIndices(bool write)
{
	CDXMesh::UnlockIndices(write);
	LeaveCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);
}

CDXBSTriShapeMesh::CDXBSTriShapeMesh()
{
	m_geometry = nullptr;
}

CDXBSTriShapeMesh::~CDXBSTriShapeMesh()
{
	
}

CDXBSTriShapeMesh * CDXBSTriShapeMesh::Create(CDXD3DDevice * pDevice, RE::BSTriShape * geometry)
{
	std::uint32_t vertCount = 0;
	std::uint32_t triangleCount = 0;

	std::uint16_t alphaFlags = 0;
	std::uint8_t alphaThreshold = 0;
	std::uint32_t shaderFlags1 = 0;
	std::uint32_t shaderFlags2 = 0;

	CDXBSTriShapeMesh * nifMesh = new CDXBSTriShapeMesh;
	nifMesh->m_geometry.reset(geometry);
	RE::BSShaderMaterial * material = nullptr;

	if (geometry)
	{
		// Pre-transform
		RE::NiTransform localTransform = GetGeometryTransform(geometry);
		const RE::BSLightingShaderProperty * shaderProperty = netimmerse_cast<RE::BSLightingShaderProperty*>(geometry->shaderProperty.get());
		if (shaderProperty) {
			material = shaderProperty->material;
			std::uint64_t sf = shaderProperty->flags.underlying();
			shaderFlags1 = static_cast<std::uint32_t>(sf & 0xFFFFFFFF);
			shaderFlags2 = static_cast<std::uint32_t>(sf >> 32);
		}

		const RE::NiAlphaProperty * alphaProperty = geometry->alphaProperty.get();
		if (alphaProperty) {
			alphaFlags = alphaProperty->alphaFlags;
			alphaThreshold = alphaProperty->alphaThreshold;
		}

		const RE::NiSkinInstance * skinInstance = geometry->skinInstance.get();
		if (!skinInstance) {
			delete nifMesh;
			return nullptr;
		}

		const RE::NiSkinPartition * skinPartition = skinInstance->skinPartition.get();
		if (!skinPartition) {
			delete nifMesh;
			return nullptr;
		}

		std::vector<CDXMeshIndex> indices;
		for (std::uint32_t p = 0; p < skinPartition->numPartitions; ++p)
		{
			for (std::uint32_t t = 0; t < skinPartition->partitions[p].triangles * 3; ++t)
			{
				indices.push_back(skinPartition->partitions[p].triList[t]);
			}
		}

		vertCount = geometry->vertexCount ? geometry->vertexCount : skinPartition->vertexCount;
		triangleCount = indices.size();

		nifMesh->m_vertCount = vertCount;
		nifMesh->m_indexCount = triangleCount;

		RE::BSFaceGenBaseMorphExtraData * morphData = (RE::BSFaceGenBaseMorphExtraData *)geometry->GetExtraData("FOD");
		if (morphData) {
			nifMesh->m_morphable = true;
		}

		nifMesh->InitializeBuffers(pDevice, nifMesh->m_vertCount, nifMesh->m_indexCount, [&](CDXMeshVert* pVertices, CDXMeshIndex* pIndices)
		{
			nifMesh->m_topology = REX::W32::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			memcpy(pIndices, &indices.at(0), indices.size() * sizeof(CDXMeshIndex));

			RE::BSDynamicTriShape * dynamicTriShape = geometry ? geometry->AsDynamicTriShape() : nullptr;
			std::uint32_t vertexSize = geometry->vertexDesc.GetSize();
			std::uint32_t vertOffset = geometry->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::Attribute::VA_POSITION);
			std::uint32_t uvOffset = geometry->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::Attribute::VA_TEXCOORD0);

			if(dynamicTriShape) dynamicTriShape->lock.Lock();
			for (std::uint32_t i = 0; i < vertCount; i++) {
				RE::NiPoint3 * vertex = dynamicTriShape ? reinterpret_cast<RE::NiPoint3*>(&reinterpret_cast<DirectX::XMFLOAT4*>(dynamicTriShape->dynamicData)[i]) : reinterpret_cast<RE::NiPoint3*>(&skinPartition->partitions[0].buffData->rawVertexData[i * vertexSize + vertOffset]);
				RE::NiPoint3 xformed = localTransform * (*vertex);
				struct UVCoord
				{
					half_float::half u;
					half_float::half v;
				};
				UVCoord * texCoord = reinterpret_cast<UVCoord*>(&skinPartition->partitions[0].buffData->rawVertexData[i * vertexSize + uvOffset]);
				DirectX::XMFLOAT2 uv{ texCoord->u, texCoord->v };
				pVertices[i].Position = *(DirectX::XMFLOAT3*)&xformed;
				pVertices[i].Normal = DirectX::XMFLOAT3(0,0,0);
				pVertices[i].Tex = uv;
				XMStoreFloat3(&pVertices[i].Color, COLOR_UNSELECTED);
			}
			if (dynamicTriShape) dynamicTriShape->lock.Unlock();
		});

		nifMesh->BuildAdjacency();
		if (nifMesh->IsMorphable()) {
			nifMesh->BuildFacemap();
			nifMesh->BuildNormals();
		}

		std::shared_ptr<CDXNifMaterial> meshMaterial = std::make_shared<CDXNifMaterial>();
		meshMaterial->SetWireframeColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		meshMaterial->SetShaderFlags1(shaderFlags1);
		meshMaterial->SetShaderFlags2(shaderFlags2);
		if (alphaFlags != 0) {
			meshMaterial->SetFlags(alphaFlags);
			meshMaterial->SetAlphaThreshold(alphaThreshold);
		}

		const RE::BSLightingShaderProperty * lightingShaderProperty = netimmerse_cast<RE::BSLightingShaderProperty*>(geometry->shaderProperty.get());
		if (lightingShaderProperty) {
			RE::BSLightingShaderMaterial * lightingMaterial = static_cast<RE::BSLightingShaderMaterial*>(material);
			RE::NiTexture * textures[] = { lightingMaterial->diffuseTexture.get(), lightingMaterial->normalTexture.get(), lightingMaterial->rimSoftLightingTexture.get() };
			for (std::uint32_t i = 0; i < sizeof(textures) / sizeof(RE::NiTexture*); ++i)
			{
				if (textures[i]) {
					meshMaterial->SetNiTexture(i, textures[i]);
				}
			}
		}

		if (material) {
			switch(material->GetFeature())
			{
				case RE::BSShaderMaterial::Feature::kFaceGen:
				{
					const RE::BSLightingShaderMaterialFacegen * tintMaterial = static_cast<RE::BSLightingShaderMaterialFacegen*>(material);
					if (tintMaterial->tintTexture) {
						meshMaterial->SetNiTexture(4, tintMaterial->tintTexture.get());
					}
					break;
				}
				case RE::BSShaderMaterial::Feature::kFaceGenRGBTint:
				{
					const RE::BSLightingShaderMaterialFacegenTint * tintMaterial = static_cast<RE::BSLightingShaderMaterialFacegenTint*>(material);
					meshMaterial->SetTintColor(DirectX::XMFLOAT4(tintMaterial->tintColor.red, tintMaterial->tintColor.green, tintMaterial->tintColor.blue, 1.0f));
					break;
				}
				case RE::BSShaderMaterial::Feature::kHairTint:
				{
					const RE::BSLightingShaderMaterialHairTint * tintMaterial = static_cast<RE::BSLightingShaderMaterialHairTint*>(material);
					meshMaterial->SetTintColor(DirectX::XMFLOAT4(tintMaterial->tintColor.red, tintMaterial->tintColor.green, tintMaterial->tintColor.blue, 1.0f));
					break;
				}
			}
		}

		nifMesh->SetMaterial(meshMaterial);
	}

	if (!nifMesh->IsMorphable())
		nifMesh->SetLocked(true);

	return nifMesh;
}

const char * CDXBSTriShapeMesh::GetName() const
{
	return m_geometry ? m_geometry->name.c_str() : "";
}

CDXLegacyNifMesh::CDXLegacyNifMesh()
{
	m_geometry = nullptr;
}

CDXLegacyNifMesh::~CDXLegacyNifMesh()
{
	
}

CDXLegacyNifMesh * CDXLegacyNifMesh::Create(CDXD3DDevice * pDevice, RE::NiGeometry * geometry)
{
	std::uint32_t vertCount = 0;
	std::uint32_t triangleCount = 0;

	REX::W32::ID3D11ShaderResourceView * diffuseTexture = nullptr;

	std::uint16_t alphaFlags = 0;
	std::uint8_t alphaThreshold = 0;
	std::uint32_t shaderFlags1 = 0;
	std::uint32_t shaderFlags2 = 0;

	CDXLegacyNifMesh * nifMesh = new CDXLegacyNifMesh;
	nifMesh->m_geometry.reset(geometry);

	if (geometry)
	{
		RE::NiGeometryData * geomDataBase = geometry->spModelData.get();
		RE::NiTriBasedGeomData * geometryData = netimmerse_cast<RE::NiTriBasedGeomData*>(geomDataBase);
		if (geometryData)
		{
			RE::NiTriShapeData * triShapeData = netimmerse_cast<RE::NiTriShapeData*>(geometryData);
			RE::NiTriStripsData * triStripsData = netimmerse_cast<RE::NiTriStripsData*>(geometryData);
			if (triShapeData || triStripsData)
			{
				// Pre-transform
				RE::NiTransform localTransform = GetLegacyGeometryTransform(geometry);
				RE::BSLightingShaderProperty * shaderProperty = netimmerse_cast<RE::BSLightingShaderProperty*>(geometry->spEffectState.get());
				if (shaderProperty) {
					RE::BSLightingShaderMaterial * material = static_cast<RE::BSLightingShaderMaterial*>(shaderProperty->material);
					if (material) {
						RE::NiTexture * diffuse = material->diffuseTexture.get();
						if (diffuse) {
							auto srcTex = static_cast<RE::NiSourceTexture*>(diffuse);
							RE::BSGraphics::Texture * rendererData = srcTex ? srcTex->rendererTexture : nullptr;
							if (rendererData) {
								diffuseTexture = rendererData->resourceView;
							}
						}
					}

					std::uint64_t sf = shaderProperty->flags.underlying();
					shaderFlags1 = static_cast<std::uint32_t>(sf & 0xFFFFFFFF);
					shaderFlags2 = static_cast<std::uint32_t>(sf >> 32);
				}

				RE::NiAlphaProperty * alphaProperty = netimmerse_cast<RE::NiAlphaProperty*>(geometry->spPropertyState.get());
				if (alphaProperty) {
					alphaFlags = alphaProperty->alphaFlags;
					alphaThreshold = alphaProperty->alphaThreshold;
				}

				vertCount = geometryData->vertices;
				triangleCount = geometryData->numTriangles;

				nifMesh->m_vertCount = vertCount;
				nifMesh->m_indexCount = triangleCount;

				RE::BSFaceGenBaseMorphExtraData * morphData = (RE::BSFaceGenBaseMorphExtraData *)geometry->GetExtraData("FOD");
				if (morphData) {
					nifMesh->m_morphable = true;
				}

				nifMesh->InitializeBuffers(pDevice, nifMesh->m_vertCount, nifMesh->m_indexCount, [&](CDXMeshVert* pVertices, CDXMeshIndex* pIndices)
				{
					if (triShapeData)
					{
						nifMesh->m_topology = REX::W32::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
						memcpy(pIndices, triShapeData->triList, triShapeData->triListLength * sizeof(CDXMeshIndex));
					}
					else if (triStripsData)
					{
						nifMesh->m_topology = REX::W32::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
						memcpy(pIndices, triStripsData->stripLists, GetStripLengthSum(triStripsData) * sizeof(CDXMeshIndex));
					}

					for (std::uint32_t i = 0; i < vertCount; i++) {
						RE::NiPoint3 xformed = localTransform * geometryData->vertex[i];
						RE::NiPoint2 uv = geometryData->texture[i];
						pVertices[i].Position = *(DirectX::XMFLOAT3*)&xformed;
						DirectX::XMFLOAT3 vNormal(0, 0, 0);
						pVertices[i].Normal = vNormal;
						pVertices[i].Tex = *(DirectX::XMFLOAT2*)&uv;
						XMStoreFloat3(&pVertices[i].Color, COLOR_UNSELECTED);

						// Build adjacency table
						if (nifMesh->m_morphable) {
							for (std::uint32_t f = 0; f < triangleCount; f++) {
								if (triShapeData) {
									CDXMeshFace * face = (CDXMeshFace *)&pIndices[f * 3];
									if (i == face->v1 || i == face->v2 || i == face->v3)
										nifMesh->m_adjacency[i].push_back(*face);
								}
								else if (triStripsData) {
									std::uint16_t v1 = 0, v2 = 0, v3 = 0;
									GetTriangleIndices(triStripsData, f, v1, v2, v3);
									if (i == v1 || i == v2 || i == v3)
										nifMesh->m_adjacency[i].push_back(CDXMeshFace(v1, v2, v3));
								}
							}
						}
					}

					// Don't need edge table if not editable
					if (nifMesh->m_morphable) {
						CDXEdgeMap edges;
						for (std::uint32_t f = 0; f < triangleCount; f++) {

							if (triShapeData) {
								CDXMeshFace * face = (CDXMeshFace *)&pIndices[f * 3];
								auto it = edges.emplace(CDXMeshEdge(std::min(face->v1, face->v2), std::max(face->v1, face->v2)), 1);
								if (it.second == false)
									it.first->second++;
								it = edges.emplace(CDXMeshEdge(std::min(face->v2, face->v3), std::max(face->v2, face->v3)), 1);
								if (it.second == false)
									it.first->second++;
								it = edges.emplace(CDXMeshEdge(std::min(face->v3, face->v1), std::max(face->v3, face->v1)), 1);
								if (it.second == false)
									it.first->second++;
							}
							else if (triStripsData) {
								std::uint16_t v1 = 0, v2 = 0, v3 = 0;
								GetTriangleIndices(triStripsData, f, v1, v2, v3);
								auto it = edges.emplace(CDXMeshEdge(std::min(v1, v2), std::max(v1, v2)), 1);
								if (it.second == false)
									it.first->second++;
								it = edges.emplace(CDXMeshEdge(std::min(v2, v3), std::max(v2, v3)), 1);
								if (it.second == false)
									it.first->second++;
								it = edges.emplace(CDXMeshEdge(std::min(v3, v1), std::max(v3, v1)), 1);
								if (it.second == false)
									it.first->second++;
							}
						}

						for (auto e : edges) {
							if (e.second == 1) {
								nifMesh->m_vertexEdges.insert(e.first.p1);
								nifMesh->m_vertexEdges.insert(e.first.p2);
							}
						}
					}

					// Only need vertex normals when it's editable
					if (nifMesh->m_morphable) {
						for (std::uint32_t i = 0; i < vertCount; i++) {
							// Setup normals
							CDXVec vNormal = XMVectorSet(0, 0, 0, 0);
							if (!geometryData->normal)
								XMStoreFloat3(&pVertices[i].Normal, nifMesh->CalculateVertexNormal(i));
							else
								XMStoreFloat3(&pVertices[i].Normal, XMLoadFloat3((XMFLOAT3*)&geometryData->normal[i]));
						}
					}
				});
				
				std::shared_ptr<CDXMaterial> material = std::make_shared<CDXMaterial>();
				material->SetTexture(0, diffuseTexture);
				material->SetWireframeColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
				material->SetShaderFlags1(shaderFlags1);
				material->SetShaderFlags2(shaderFlags2);
				if (alphaFlags != 0) {
					material->SetFlags(alphaFlags);
					material->SetAlphaThreshold(alphaThreshold);
				}

				nifMesh->m_material = material;
			}
		}
	}

	if (!nifMesh->m_morphable)
		nifMesh->SetLocked(true);

	return nifMesh;
}

const char * CDXLegacyNifMesh::GetName() const
{
	return m_geometry ? m_geometry->name.c_str() : "";
}

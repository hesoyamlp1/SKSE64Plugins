#include "AttachmentInterface.h"
#include "SKEETasks.h"
#include "BodyMorphInterface.h"
#include "OverrideInterface.h"


#include "RE/N/NiGeometry.h"

#include "NifUtils.h"
#include <cstdint>

extern const SKSE::TaskInterface* g_task;
extern AttachmentInterface	g_attachmentInterface;
extern BodyMorphInterface	g_bodyMorphInterface;
extern OverrideInterface	g_overrideInterface;
const char* AttachmentInterface::ATTACHMENT_HOLDER = "NiAttachments [NiOA]";

SKSEAttachSkinnedMesh::SKSEAttachSkinnedMesh(RE::TESObjectREFR* ref, const RE::BSFixedString& nifPath, const RE::BSFixedString& name, bool firstPerson, bool replace, const std::vector<RE::BSFixedString>& filter)
	: m_formId(ref->formID)
	, m_nifPath(nifPath)
	, m_name(name)
	, m_firstPerson(firstPerson)
	, m_replace(replace)
	, m_filter(filter)
{

}

void SKSEDetachSkinnedMesh::Run()
{
	RE::TESForm* form = RE::TESForm::LookupByID(m_formId);
	if (!form) {
		return;
	}

	if (form->IsNot(RE::FormType::ActorCharacter) && form->IsNot(RE::FormType::Reference)) {
		return;
	}

	g_attachmentInterface.DetachMesh(static_cast<RE::TESObjectREFR*>(form), m_name.c_str(), m_firstPerson);
}

SKSEDetachSkinnedMesh::SKSEDetachSkinnedMesh(RE::TESObjectREFR* ref, const RE::BSFixedString& name, bool firstPerson)
	: m_formId(ref->formID)
	, m_name(name)
	, m_firstPerson(firstPerson)
{

}

void SKSEAttachSkinnedMesh::Run()
{
	RE::TESForm* form = RE::TESForm::LookupByID(m_formId);
	if (!form) {
		return;
	}

	if (form->IsNot(RE::FormType::ActorCharacter) && form->IsNot(RE::FormType::Reference)) {
		return;
	}

	RE::TESObjectREFR* reference = static_cast<RE::TESObjectREFR*>(form);
	RE::NiAVObject* outRoot = nullptr;

	const char** filter = nullptr;
	if (m_filter.size())
	{
		filter = new const char* [m_filter.size()];
		for (size_t i = 0; i < m_filter.size(); ++i)
		{
			filter[i] = m_filter[i].c_str();
		}
	}

	if (g_attachmentInterface.AttachMesh(reference, m_nifPath.c_str(), m_name.c_str(), m_firstPerson, m_replace, filter, m_filter.size(), outRoot, nullptr, 0) && form->IsActor())
	{
		g_bodyMorphInterface.ApplyVertexDiff(reference, outRoot);
		g_overrideInterface.Impl_ApplyNodeOverrides(reference, outRoot, true);
	}

	if (filter)
	{
		delete[] filter;
	}
}

void SKSEDetachAllSkinnedMeshes::Run()
{
	RE::TESForm* form = RE::TESForm::LookupByID(m_formId);
	if (!form) {
		return;
	}

	if (form->IsNot(RE::FormType::ActorCharacter) && form->IsNot(RE::FormType::Reference)) {
		return;
	}

	RE::TESObjectREFR* reference = static_cast<RE::TESObjectREFR*>(form);

	VisitSkeletalRoots(reference, [&](RE::NiNode* rootNode, bool isFirstPerson)
	{
		RE::BSFixedString parentName("NPC Root [Root]");
		RE::NiAVObject* destination = rootNode->GetObjectByName(parentName);
		if (destination) {
			RE::NiNode* destinationNode = destination ? destination->AsNode() : nullptr;
			if (destinationNode) {
				RE::BSFixedString holderName(AttachmentInterface::ATTACHMENT_HOLDER);
				auto holderNode = destinationNode->GetObjectByName(holderName);
				if (holderNode) {
					destinationNode->DetachChild(holderNode);
				}
			}
		}
	});
}

void AttachmentInterface::Revert()
{
	if (auto* task = SKSE::GetTaskInterface())
	{
		m_attachedLock.lock();
		for (auto& formId : m_attached)
		{
			SKEE_AddTask(task, new SKSEDetachAllSkinnedMeshes(formId));
		}
		m_attachedLock.unlock();
	}
}

bool AttachmentInterface::AttachMesh(RE::TESObjectREFR* ref, const char* path, const char* nodeName, bool firstPerson, bool replace, const char** filter, skee_u32 filterSize, RE::NiAVObject*& out, char* errBuf, skee_u64 errBufLen)
{
	RE::NiNode* root = ref->Get3D(firstPerson) ? ref->Get3D(firstPerson)->AsNode() : nullptr;
	if (!root) {
		return false;
	}

	float weight = 100.0;
	RE::TESNPC* npc = ref->GetBaseObject() ? ref->GetBaseObject()->As<RE::TESNPC>() : nullptr;
	if (npc && npc->faceNPC) {
		// Legacy GetRootTemplate(): follow the face chain to its root for the effective weight.
		// GetRootFaceNPC() cannot return null here (it starts at `this`).
		weight = npc->GetRootFaceNPC()->weight;
	} else {
		weight = ref->GetWeight();
	}
	weight /= 100.0;

	DirectX::XMVECTOR weightScale = DirectX::XMVectorReplicate(weight);

	RE::BSFixedString parentName("NPC Root [Root]");
	RE::NiAVObject* rootDestination = root->GetObjectByName(parentName);
	if (!rootDestination) {
		return false;
	}

	RE::NiNode* rootNode = rootDestination ? rootDestination->AsNode() : nullptr;
	if (!rootNode) {
		return false;
	}

	bool createdHolder = false;
	RE::BSFixedString holderName(ATTACHMENT_HOLDER);

	RE::NiPointer<RE::NiAVObject> holder(rootDestination->GetObjectByName(holderName));
	if (!holder)
	{
		holder = RE::NiPointer<RE::NiAVObject>(RE::NiNode::Create(0));
		holder->name = holderName;
		createdHolder = true;
	}

	RE::NiNode* holderNode = holder.get() ? holder.get()->AsNode() : nullptr;
	if (!holderNode) {
		return false;
	}

	RE::BSFixedString targetNodeName(nodeName);
	RE::NiAVObject* target = holderNode->GetObjectByName(targetNodeName);
	if (target) {
		if (replace)
		{
			holderNode->DetachChild(target);
		}
		else
		{
			// Nothing to do, the mesh is already here
			return true;
		}
	}
	
	RE::BSFixedString nifPath(path);
	NifStreamWrapper niStream;
	RE::BSResourceNiBinaryStream binaryStream(nifPath.c_str());
	if (!binaryStream.good()) {
		std::snprintf(errBuf, errBufLen, "No file exists at (%s)\n", path);
		return false;
	}

	if (!niStream.LoadStream(&binaryStream)) {
		std::snprintf(errBuf, errBufLen, "File at (%s) is not a valid nif\n", path);
		return false;
	}

	// Try searching for the _0 version of the mesh
	NifStreamWrapper lowStream;
	std::string lowPath(path);
	RE::NiNode* lowRoot = nullptr;
	size_t foundWeight = lowPath.find("_1");
	if (foundWeight != std::string::npos)
	{
		lowPath[foundWeight + 1] = '0';
		RE::BSFixedString lowFile(lowPath.c_str());

		RE::BSResourceNiBinaryStream lowBinaryStream(lowFile.c_str());
		if (lowBinaryStream.good() && lowStream.LoadStream(&lowBinaryStream)) {
			lowStream.VisitObjects([&lowRoot](RE::NiObject* object)
			{
				auto* node = object ? object->AsNode() : nullptr;
				if (node) {
					lowRoot = node;
					return true;
				}

				return false;
			});
		}
	}

	std::unordered_set<RE::BSFixedString> filteredNames;
	for (std::uint32_t i = 0; i < filterSize; ++i)
	{
		filteredNames.emplace(filter[i]);
	}

	RE::NiNode* loadedRoot = nullptr;
	niStream.VisitObjects([&loadedRoot](RE::NiObject* object)
	{
		auto* node = object ? object->AsNode() : nullptr;
		if (node) {
			loadedRoot = node;
			return true;
		}

		return false;
	});

	if (!loadedRoot) {
		return false;
	}

	RE::NiPointer<RE::NiNode> targetRoot(RE::NiNode::Create(0));
	targetRoot->name = targetNodeName;

	int c = 0;
	if (VisitGeometry(loadedRoot, [&](RE::BSGeometry* geometry)
	{
		if (filteredNames.count(geometry->name))
		{
			return false;
		}

		targetRoot->AttachChild(geometry, true);

		auto skinInstance = geometry->skinInstance;
		if (skinInstance) {
			auto skinData = skinInstance->skinData;
			if (!skinData) {
				c += std::snprintf(errBuf + c, std::max(0LL, static_cast<std::int64_t>(errBufLen) - c), "Error attaching skinned mesh to ref (%08X) nif (%s) shape (%s) has no skin data\n", ref->formID, path, geometry->name.c_str());
				return true;
			}

			auto skinPartition = skinInstance->skinPartition;
			if (skinPartition && lowRoot)
			{
				RE::NiAVObject* weightedGeom = lowRoot->GetObjectByName(geometry->name);
				if (weightedGeom) {
					RE::BSGeometry* lowGeometry = weightedGeom ? weightedGeom->AsGeometry() : nullptr;
					if (lowGeometry) {
						auto lowSkinInstance = lowGeometry->skinInstance;
						if (lowSkinInstance) {
							auto lowSkinPartition = lowSkinInstance->skinPartition;
							if (lowSkinPartition) {
								std::uint32_t hiVerts = skinPartition->vertexCount;
								std::uint32_t loVerts = lowSkinPartition->vertexCount;

								if (geometry->vertexDesc.GetFlags() == lowGeometry->vertexDesc.GetFlags() && geometry->vertexDesc.GetSize() == lowGeometry->vertexDesc.GetSize() && hiVerts == loVerts) {
									bool hasVertices = geometry->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_VERTEX);
									if (hasVertices) {
										std::uint32_t vertexSize = geometry->vertexDesc.GetSize();
										std::uint8_t* hiBlock = reinterpret_cast<std::uint8_t*>(skinPartition->partitions[0].buffData->rawVertexData);
										std::uint8_t* loBlock = reinterpret_cast<std::uint8_t*>(lowSkinPartition->partitions[0].buffData->rawVertexData);
										for (std::uint32_t i = 0; i < hiVerts; ++i)
										{
											DirectX::XMVECTOR hiVertex = *reinterpret_cast<DirectX::XMVECTOR*>(&hiBlock[i * vertexSize]);
											DirectX::XMVECTOR loVertex = *reinterpret_cast<DirectX::XMVECTOR*>(&loBlock[i * vertexSize]);
											DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&hiBlock[i * vertexSize]), DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSubtract(hiVertex, loVertex), weightScale, loVertex));
										}

										// Update the resources and propagate to duplicate partitions
										NIOVTaskUpdateSkinPartition update(skinInstance.get(), skinPartition.get());
										update.Run();
									}
								}
							}
						}
					}
				}
			}

			for (std::uint32_t i = 0; i < skinData->GetBoneCount(); ++i)
			{
				auto bone = skinInstance->bones[i];
				if (!bone) {
					c += std::snprintf(errBuf + c, std::max(0LL, static_cast<std::int64_t>(errBufLen) - c), "Error attaching skinned mesh to ref (%08X) nif (%s) shape (%s) has invalid bone at index (%d)\n", ref->formID, path, geometry->name.c_str(), i);
					return true;
				}

				auto boneNode = root->GetObjectByName(bone->name);
				if (!boneNode) {
					c += std::snprintf(errBuf + c, std::max(0LL, static_cast<std::int64_t>(errBufLen) - c), "Error attaching skinned mesh to ref (%08X) nif (%s) shape (%s) missing bone (%s) on ref root skeleton\n", ref->formID, path, geometry->name.c_str(), bone->name.c_str());
					return true;
				}

				skinInstance->bones[i] = boneNode;
				skinInstance->boneWorldTransforms[i] = &boneNode->world;
				skinInstance->rootParent = root;
			}
		}

		return false;
	}))
	{
		return false;
	}

	holderNode->AttachChild(targetRoot.get(), false);
	if (createdHolder)
	{
		rootNode->AttachChild(holderNode, false);
	}

	m_attachedLock.lock();
	m_attached.emplace(ref->formID);
	m_attachedLock.unlock();

	out = targetRoot.get();
	return true;
}

bool AttachmentInterface::DetachMesh(RE::TESObjectREFR* ref, const char* nodeName, bool firstPerson)
{
	RE::NiNode* root = ref->Get3D(firstPerson) ? ref->Get3D(firstPerson)->AsNode() : nullptr;
	if (!root) {
		return false;
	}

	RE::BSFixedString parentName("NPC Root [Root]");
	RE::NiAVObject* destination = root->GetObjectByName(parentName);
	if (!destination) {
		return false;
	}

	RE::BSFixedString holderName(ATTACHMENT_HOLDER);
	destination = destination->GetObjectByName(holderName);
	if (!destination) {
		return false;
	}

	RE::NiNode* destinationNode = destination ? destination->AsNode() : nullptr;
	if (!destinationNode) {
		return false;
	}

	RE::BSFixedString targetNodeName(nodeName);
	RE::NiAVObject* target = destinationNode->GetObjectByName(targetNodeName);
	if (!target) {
		return false;
	}

	destinationNode->DetachChild(target);
	return true;
}


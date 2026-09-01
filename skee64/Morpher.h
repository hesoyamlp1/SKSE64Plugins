#pragma once

#include "shape.hpp"
#include "kd_matcher.hpp"

#include <vector>
#include <functional>
#include <unordered_set>

#include <RE/B/BSGeometry.h>
#include <RE/N/NiSkinPartition.h>
#include <RE/N/NiSmartPointer.h>
#include <cstdint>

class NormalApplicator
{
public:
	NormalApplicator(RE::NiPointer<RE::BSGeometry> _geometry, RE::NiPointer<RE::NiSkinPartition> _skinPartition);

	void Apply();

	void RecalcNormals(std::uint32_t numTriangles, Morpher::Triangle* triangles, const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace(std::uint32_t numTriangles, Morpher::Triangle * triangles);

protected:
	RE::NiPointer<RE::BSGeometry> geometry;
	RE::NiPointer<RE::NiSkinPartition> skinPartition;
	std::unordered_set<std::uint16_t> lockedVertices;
	std::vector<Morpher::Vector3> rawVertices;
	std::vector<Morpher::Vector3> rawNormals;
	std::vector<Morpher::Vector2> rawUV;
	std::vector<Morpher::Vector3> rawTangents;
	std::vector<Morpher::Vector3> rawBitangents;
};

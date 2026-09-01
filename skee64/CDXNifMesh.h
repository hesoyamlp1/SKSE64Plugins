#ifndef __CDXNIFMESH__
#define __CDXNIFMESH__

#pragma once

#include "CDXEditableMesh.h"
#include "CDXMaterial.h"

#include <RE/B/BSTriShape.h>
#include <RE/N/NiGeometry.h>
#include <RE/N/NiGeometryData.h>
#include <RE/N/NiSmartPointer.h>

class CDXScene;
class CDXShader;
class CDXD3DDevice;

class CDXNifMesh : public CDXEditableMesh
{
public:
	CDXNifMesh();
	virtual ~CDXNifMesh();

	virtual const char* GetName() const override { return ""; }
	virtual RE::NiGeometry* GetLegacyGeometry() { return nullptr; }
	virtual RE::BSTriShape* GetGeometry() { return nullptr; }
	virtual bool IsMorphable() const
	{
#ifdef CDX_MUTEX
		std::lock_guard<std::mutex> guard(m_mutex);
#endif
		return m_morphable;
	}

	virtual CDXMeshVert * LockVertices(const LockMode type = READ) override;
	virtual CDXMeshIndex * LockIndices() override;

	virtual void UnlockVertices(const LockMode type) override;
	virtual void UnlockIndices(bool write = false) override;

protected:
	bool m_morphable;
};

class CDXLegacyNifMesh : public CDXNifMesh
{
public:
	CDXLegacyNifMesh();
	virtual ~CDXLegacyNifMesh();

	static CDXLegacyNifMesh * Create(CDXD3DDevice * pDevice, RE::NiGeometry * geometry);
	virtual const char* GetName() const override;
	virtual RE::NiGeometry* GetLegacyGeometry() override
	{
#ifdef CDX_MUTEX
		std::lock_guard<std::mutex> guard(m_mutex);
#endif
		return m_geometry.get();
	}

private:
	RE::NiPointer<RE::NiGeometry> m_geometry;
};

class CDXBSTriShapeMesh : public CDXNifMesh
{
public:
	CDXBSTriShapeMesh();
	virtual ~CDXBSTriShapeMesh();

	static CDXBSTriShapeMesh * Create(CDXD3DDevice * pDevice, RE::BSTriShape * geometry);

	virtual const char* GetName() const override;
	virtual RE::BSTriShape* GetGeometry() override
	{
#ifdef CDX_MUTEX
		std::lock_guard<std::mutex> guard(m_mutex);
#endif
		return m_geometry.get();
	}

private:
	RE::NiPointer<RE::BSTriShape> m_geometry;
};

#endif
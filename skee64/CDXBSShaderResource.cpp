#include "CDXBSShaderResource.h"
#include "RE/B/BSResourceNiBinaryStream.h"
#include "FileUtils.h"

CDXBSShaderResource::CDXBSShaderResource(const char * filePath, const char * entryPoint)
{
	const char * shaderPath = "SKSE/Plugins/NiOverride/texture.fx";

	std::vector<char> vsb;
	RE::BSResourceNiBinaryStream vs(filePath);
	if (!vs.good())
	{
		SKSE::log::error("{} - Failed to read {}", __FUNCTION__, filePath);
	}
	else
	{
		BSFileUtil::ReadAll(&vs, m_fileBuffer);
		m_sourceName = filePath;
		m_entryPoint = entryPoint;
	}
}

void * CDXBSShaderResource::GetBuffer() const
{
	return m_fileBuffer.size() > 0 ? (void*)&m_fileBuffer.at(0) : nullptr;
}

size_t CDXBSShaderResource::GetBufferSize() const
{
	return m_fileBuffer.size();
}

const char * CDXBSShaderResource::GetSourceName() const
{
	return m_sourceName.c_str();
}

const char * CDXBSShaderResource::GetEntryPoint() const
{
	return m_entryPoint.c_str();
}
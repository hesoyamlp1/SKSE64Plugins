#ifndef __CDXUNDO__
#define __CDXUNDO__

#pragma once

#include "CDXTypes.h"

#include <vector>
#include <memory>
#include <cstdint>

class CDXUndoCommand
{
public:
	virtual ~CDXUndoCommand() { };

	enum UndoType
	{
		kUndoType_None = 0,
		kUndoType_Stroke,
		kUndoType_ResetMask,
		kUndoType_ResetSculpt,
		kUndoType_Import
	};

	virtual UndoType GetUndoType() { return kUndoType_None; }
	virtual void Undo() { };
	virtual void Redo() { };
};

typedef std::shared_ptr<CDXUndoCommand> CDXUndoCommandPtr;

class CDXUndoStack : public std::vector<CDXUndoCommandPtr>
{
public:
	CDXUndoStack();

	std::int32_t Undo(bool doUpdate);
	std::int32_t Redo(bool doUpdate);
	std::int32_t GoTo(std::int32_t index, bool doUpdate);
	std::int32_t Push(CDXUndoCommandPtr action);
	std::int32_t GetIndex() const { return m_index; }

	std::uint32_t GetLimit() const { return m_maxStack; }

	void Release();

protected:
	std::int32_t	m_index;
	std::uint32_t	m_maxStack;
};

extern CDXUndoStack	g_undoStack;

#endif

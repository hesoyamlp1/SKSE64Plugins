#pragma once

// Project-local locked data holder (legacy skse64 GameTypes.h SafeDataHolder).
// The lock is recursive to match the legacy CRITICAL_SECTION semantics.

#include <mutex>

template <typename T>
class SafeDataHolder
{
protected:
	mutable std::recursive_mutex m_lock;

public:
	T m_data;

	void Lock() const   { m_lock.lock(); }
	void Release() const { m_lock.unlock(); }
};

template <typename T>
class SafeDataLocker
{
public:
	explicit SafeDataLocker(SafeDataHolder<T>& a_holder) : m_holder(a_holder)
	{
		m_holder.Lock();
	}
	~SafeDataLocker() { m_holder.Release(); }

private:
	SafeDataHolder<T>& m_holder;
};

#ifndef XCGUI_UITOOL_SHADOW_INTERNAL_H
#define XCGUI_UITOOL_SHADOW_INTERNAL_H
// CXShadow 模块级全局状态 — InitOnce + SRWLOCK, 仅 module_xcgui_uitool.cpp 定义.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <set>
#include <unordered_map>

#include "module_xcgui.h"

class CXShadow;

namespace CXShadowModule {

void EnsureInitialized();

std::unordered_map<HWINDOW, CXShadow*>& HostMap();
std::set<CXShadow*>& Instances();
int& GlobalTheme();

struct HostLock {
	HostLock();
	~HostLock();
	HostLock(const HostLock&) = delete;
	HostLock& operator=(const HostLock&) = delete;
};

struct InstancesLock {
	InstancesLock();
	~InstancesLock();
	InstancesLock(const InstancesLock&) = delete;
	InstancesLock& operator=(const InstancesLock&) = delete;
};

} // namespace CXShadowModule

#endif // XCGUI_UITOOL_SHADOW_INTERNAL_H

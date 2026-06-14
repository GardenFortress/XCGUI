// module_xcgui_uitool.cpp — TU 聚合入口
#define _XCGUI_UITOOL_AGGREGATED_
//
// 顶层类 split:
//   CXTooltip / CXLoading / CXCalendarCard / CXShadow / CXEditDW / CXChatBubbleBox / CXBlur / CXAccordion / CXCardPanel / CXSteps / CXColorPicker (+ dcomp)
// 共有:
//   module_xcgui_uitool_common.inc      — 系统环境 + _XUITool 主题层
//   module_xcgui_uitool_steps_const.inc — CXSteps 布局/动画常量
//   module_xcgui_draw_softshadow.inc  — 软阴影描边内核
//   module_xcgui_uitool_svgs.inc        — 月历导航 + tooltip 图标 SVG

#include "module_xcgui_uitool.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "D2d1.lib")

#ifndef SafeRelease
#define SafeRelease(p) do { if (p) { (p)->Release(); (p) = NULL; } } while (0)
#endif

#include "module_xcgui_uitool_svgs.inc"
#include "module_xcgui_uitool_common.inc"
#include "module_xcgui_draw_softshadow.inc"
#include "module_xcgui_uitool_tooltip.cpp"
#include "module_xcgui_uitool_loading.cpp"
#include "module_xcgui_uitool_calendar.cpp"
#include "module_xcgui_uitool_colorpicker.cpp"
#include "module_xcgui_uitool_shadow.inc"
#include "module_xcgui_uitool_editdw.cpp"
#include "module_xcgui_uitool_chat.cpp"
#include "module_xcgui_uitool_accordion.cpp"
#include "module_xcgui_uitool_cardpanel.cpp"
#include "module_xcgui_uitool_steps.cpp"
#include "module_xcgui_uitool_blur.cpp"
#include "module_xcgui_uitool_blur_dcomp.cpp"

//============================================================================
// CXShadow 全局状态 — InitOnce 延迟初始化 + SRWLOCK (不用 std::mutex 静态对象).
// MSVC 下文件作用域 std::mutex 在模块/聚合 TU 链接时 lock 会 0xC0000005.
//============================================================================
#include "module_xcgui_uitool_shadow_internal.h"

namespace CXShadowModule {
namespace {

struct State {
	SRWLOCK                                 hostLock;
	std::unordered_map<HWINDOW, CXShadow*>  hostMap;
	SRWLOCK                                 instLock;
	std::set<CXShadow*>                     instances;
	int                                     globalTheme;
};

State* volatile s_state = nullptr;

State* CreateState()
{
	State* p = new (std::nothrow) State();
	if (!p) return nullptr;
	::InitializeSRWLock(&p->hostLock);
	::InitializeSRWLock(&p->instLock);
	p->globalTheme = xshadow_theme_custom;
	return p;
}

State& S()
{
	State* p = s_state;
	if (!p){
		State* n = CreateState();
		if (!n) __fastfail(1);
		if (::InterlockedCompareExchangePointer(
				reinterpret_cast<PVOID volatile*>(&s_state),
				n, nullptr) != nullptr){
			delete n;
		}
		p = s_state;
	}
	return *p;
}

} // namespace

void EnsureInitialized()
{
	(void)S();
}

std::unordered_map<HWINDOW, CXShadow*>& HostMap() { return S().hostMap; }
std::set<CXShadow*>& Instances() { return S().instances; }
int& GlobalTheme() { return S().globalTheme; }

HostLock::HostLock()  { ::AcquireSRWLockExclusive(&S().hostLock); }
HostLock::~HostLock() { ::ReleaseSRWLockExclusive(&S().hostLock); }

InstancesLock::InstancesLock()  { ::AcquireSRWLockExclusive(&S().instLock); }
InstancesLock::~InstancesLock() { ::ReleaseSRWLockExclusive(&S().instLock); }

} // namespace CXShadowModule

//============================================================================
// module_xcgui_uitool_steps_const.inc — CXSteps layout / anim constants
// Included by module_xcgui_uitool_steps.cpp only.
//============================================================================
#ifndef _XCGUI_UITOOL_STEPS_CONST_INC_
#define _XCGUI_UITOOL_STEPS_CONST_INC_

enum {
	kSteps_LabelSize      = 28,
	kSteps_LabelIconSize  = 20,   // kSteps_LabelSize - 8, 圆内图标/SVG 绘制尺寸
	kSteps_TextH          = 28,
	kSteps_TextFontPt     = 10,
	kSteps_LabelFontPt    = 10,
	kSteps_ConnectorW     = 3,
	kSteps_ItemGapH       = 8,
	kSteps_ItemGapV       = 10,
	kSteps_TextPadH       = 4,
	kSteps_MinItemW       = 48,
	kSteps_MinBarH        = 64,
	kSteps_VTextColMinW   = 64,
	kSteps_VTextColMaxW   = 260,
	kSteps_AnimTimerId    = 1,
	kSteps_AnimTickMs     = 16,
	kSteps_AnimDurationMs = 300,
};

#endif // _XCGUI_UITOOL_STEPS_CONST_INC_

#ifndef HEAD_A0650D85_F4BF_466d_9756_8FF69AEFA7D6
#define HEAD_A0650D85_F4BF_466d_9756_8FF69AEFA7D6
class 窗口模糊类;
class 主窗口类;
class 窗口模糊类
{
public:
public:
	void  置模糊通透度(float 模糊层通透感=0.07f);
	void  启用Snap(BOOL 是否启用);
	void  清理();
private:
	HWINDOW m_hWindow= NULL;
	CXBlur blur;
	CXShadow shadow;
	void  初始化();
public:
	BOOL  初始化(HWINDOW hWindow);
};
class 主窗口类  :  public  CXWindow
{
public:
	CXButton _按钮1;
	CXLayout _布局元素1;
	CXButton _按钮2;
	CXButton _显示双日期;
	CXLayout _布局元素2;
	CXLayout _布局元素3;
	CXButton _按钮5;
	CXText _布局文件= L"布局文件\\main.xml";
	CXShadow sd;
	窗口模糊类 blur;
	CXAccordion acc;
	CXCardPanel panl;
	CXSteps steps;
	CXColorPicker color;
	void  折叠面板测试();
	void  卡片面板测试();
	void  炫彩步骤条类测试();
	void  Test();
	void  颜色选择器();
	int  运行();
	int  按钮点击_按钮2(HELE 来源句柄, BOOL* 是否拦截);
	int  按钮点击_显示双日期(HELE 来源句柄, BOOL* 是否拦截);
	int  按钮点击_按钮5(HELE 来源句柄, BOOL* 是否拦截);
};
int WINAPI wWinMain(HINSTANCE 模块句柄, HINSTANCE 先前句柄, wchar_t* 命令行, int 窗口显示标识);
#endif

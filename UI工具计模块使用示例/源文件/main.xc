整型 g_build = 110
类 窗口模糊类
	公开
	函数 置模糊通透度(浮点型 模糊层通透感 = 0.07f)
		blur.置模糊通透度(模糊层通透感)

	函数 启用Snap(逻辑型 是否启用)
		如果 (炫彩_是否窗口(m_hWindow))
			blur.启用Snap(m_hWindow, 是否启用)

	函数 清理()
		// 必须在 XCGUI 窗口句柄仍有效时解除。类成员的 C++ 析构晚于炫彩_退出()，
		// 若依赖析构器自动 Detach，Win11 DComp 与 Win7 shadow 降级分支都可能访问已销毁句柄。
		blur.解除绑定()
		shadow.解除绑定()
		m_hWindow = NULL

	私有
	窗口句柄 m_hWindow = NULL
	CXBlur blur
	CXShadow shadow

	函数 初始化()
		blur.置主题(UI工具主题_自动)
		消息框(炫彩_整数到文本W(g_build))
		如果 (g_build == 11)
			blur.附加窗口扩展(m_hWindow)
			//炫彩亚克力模糊类::启用原生阴影(m_hWindow, 真)
		否则
			窗口_置透明类型(m_hWindow, 窗口透明标识_透明)
			窗口_置透明度(m_hWindow, 255)
			shadow.启用Snap阻止(真)
			shadow.置圆角(10)
			shadow.置描边宽(1.0f)
			shadow.附加窗口(m_hWindow)
			占位
	
	公开
	函数 逻辑型 初始化(窗口句柄 hWindow)
		如果 (!炫彩_是否窗口(hWindow))
			返回 FALSE
		m_hWindow = hWindow
		初始化()
		返回 真



类 主窗口类 继承 窗口类
	[绑定信息]
		[按钮类, _按钮1, "按钮1"]
		[布局类, _布局元素1, "布局元素1"]
		[按钮类, _按钮2, "按钮2"]
			[元素事件_按钮点击, 按钮点击_按钮2, 1]
		[按钮类, _显示双日期, "显示双日期"]
			[元素事件_按钮点击, 按钮点击_显示双日期, 1]
		[布局类, _布局元素2, "布局元素2"]
		[布局类, _布局元素3, "布局元素3"]
		[按钮类, _按钮5, "按钮5"]
			[元素事件_按钮点击, 按钮点击_按钮5, 1]

	文本型  _布局文件 = "布局文件\\main.xml"
	CXShadow sd
	窗口模糊类 blur
	炫彩折叠面板类 acc
	炫彩卡片面板类 panl
	CXSteps steps
	CXColorPicker color
	
	函数 折叠面板测试()
		acc.创建(_句柄)
		acc.置主题(UI工具主题_深色)
		acc.置分组标题对齐(折叠面板分组标题对齐_左)
		
		int g1 = acc.添加分组("折叠面板1")
		
		// 项1: 纯文本展开内容
		int i1 = acc.添加项(g1, "危险的项")
		acc.置项徽章(i1, "错误危险的")
		acc.置项徽章(i1, "错误:0xC000005", 折叠面板徽章类型_危险)
		acc.置项正文(i1, "测试内容:抢先版已更新，可直接使用。")
		
		// 项2: 元素展开内容
		int i2 = acc.添加项(g1, "成功安全的")
		acc.置项图标(i2, 折叠面板图标类型_状态完成)
		acc.置项徽章(i2, "成功安全的", 折叠面板徽章类型_成功)
		HELE hPanel = 按钮_创建(0, 0, 100, 200, "BUTTON")
		acc.置项内容元素(i2, hPanel)
		
		acc.展开项(i1)   // 点击项瞬间展开，其它项动画收起
		
		int i3 = acc.添加项(g1, "中性的项")
		acc.置项徽章(i3, "中性一般的", 折叠面板徽章类型_中性)
		acc.置项正文(i3, "测试内容:抢先版已更新，可直接使用。")
		
		int g2 = acc.添加分组("折叠面板2")
		
		int i4 = acc.添加项(g2, "信息的项")
		acc.置项徽章(i4, "信息项的内容", 折叠面板徽章类型_信息)
		acc.置项正文(i4, "测试内容:抢先版已更新，可直接使用。")
		
		int i5 = acc.添加项(g2, "警告的项")
		acc.置项徽章(i5, "警告项的内容", 折叠面板徽章类型_警告)
		acc.置项正文(i5, "测试内容:抢先版已更新，可直接使用。")
		
		int i6 = acc.添加项(g2, "无徽章标签的项")
		acc.置项正文(i6, "测试内容:抢先版已更新，可直接使用。")

	函数 卡片面板测试()
		panl.创建(_句柄)
		panl.置内填充大小(16, 0, 16, 0)
		panl.置主题(UI工具主题_浅色)
		panl.置分组标题对齐(卡片面板分组标题对齐_左)
		//panl.置圆角(10)
		panl.启用滚动(TRUE)
		
		int g1 = panl.AddGroup("系统设置")
		HELE hRow = 按钮_创建(0, 0, 0, 48, "BUTTON1")
		元素_启用背景透明(hRow, 真)
		//HELE hRow = (HELE)形状文本_创建(0, 0, 0, 48, "在系统右键菜单增加「通过QQ发送」选项")
		窗口组件_布局项_置宽度(hRow, 布局大小类型_填充父, 0)
		窗口组件_布局项_置高度(hRow, 布局大小类型_固定, 46)
		panl.置分组内容元素(g1, hRow)

		hRow = (HELE)形状文本_创建(0, 0, 0, 48, "可将本地文件便捷发送给我的手机、QQ好友或进行闪传发送")
		//hRow = 按钮_创建(0, 0, 0, 48, "BUTTON2")
		//元素_启用背景透明(hRow, 真)
		//窗口组件_布局项_启用换行(hRow, 真)
		窗口组件_布局项_置宽度(hRow, 布局大小类型_填充父, 0)
		窗口组件_布局项_置高度(hRow, 布局大小类型_固定, 100)
		panl.AddGroupContentEle(g1, hRow)

		int g2 = panl.AddGroup("系统设置2")
		hRow = 按钮_创建(0, 0, 0, 48, "BUTTON1")
		//元素_启用背景透明(hRow, 真)
		窗口组件_布局项_启用换行(hRow, 真)
		窗口组件_布局项_置宽度(hRow, 布局大小类型_填充父, 0)
		窗口组件_布局项_置高度(hRow, 布局大小类型_固定, 46)
		panl.置分组内容元素(g2, hRow)

	函数 炫彩步骤条类测试()
		steps.Create(_句柄)
		steps.SetTheme(UI工具主题_自定义)
		steps.置方向(步骤条方向_垂直)
		steps.置内容顺序(步骤条内容顺序_标签在前)

		//steps.置文本颜色(RGBA(255, 0, 0, 255))
		//steps.置背景颜色(RGBA(0, 255, 0, 255))
		//steps.置强调颜色(RGBA(0, 0, 255, 255))
		//steps.置激活文本颜色(RGBA(0xF5, 0xF5, 0xF5, 255))
		//steps.置未激活文本颜色(RGBA(0x7D, 0x7E, 0x7F, 255))
		//steps.置激活填充颜色(RGBA(0x37, 0x7A, 0xF6, 255))
		//steps.置未激活填充颜色(RGBA(0x3A, 0x3A, 0x3C, 255))
		//steps.置激活标签文本颜色(RGBA(255, 255, 255, 255))
		//steps.置未激活标签文本颜色(RGBA(0x7D, 0x7E, 0x7F, 255))
		//steps.置未激活连接线颜色(RGBA(0x4A, 0x4A, 0x4C, 255))
		
		steps.启用过渡动画(真)
		//steps.置过渡动画时长(300)
		steps.添加步骤("Register")
		steps.添加步骤("Choose plan")
		steps.添加步骤("Purchase")
		steps.添加步骤("Receive Product")
		steps.置当前步骤(1)    // 高亮 step 0、1；0–1 连接线为强调色
		steps.调整布局()

	函数 Test()
		//CXTooltip::置主题(_按钮1._句柄, UI工具主题_深色)
		//CXTooltip::添加元素提示(_按钮1._句柄, "测试🚫啊")
		//CXTooltip::置类型(_按钮1._句柄, 提示类型_信息)
		//CXTooltip::置显示延迟(_按钮1._句柄, 0)
		//CXTooltip::置渐变时长(_按钮1._句柄, 150)
		////CXTooltip::置箭头方向(_按钮1._句柄, 提示箭头方向_左)
		//CXTooltip::置显示箭头(_按钮1._句柄, 真)
		//CXTooltip::置箭头方向(_按钮1._句柄, 提示箭头方向_自动)

		//// 1. 元素附着
		//窗口句柄 hPanel = _句柄
		////HELE hPanel = _布局元素1._句柄
		//CXLoading::附加窗口(_句柄)
		//CXLoading::SetStyle(hPanel, 加载样式_频谱)
		//CXLoading::置主题(hPanel, UI工具主题_浅色)
		//CXLoading::置字号(hPanel, 20)
		//CXLoading::SetText(hPanel, "加载中... 小贴士: 第一次加载较慢")
		//CXLoading::置文本颜色(hPanel, 0x33ffffff)
		//CXLoading::置尺寸(hPanel, 20, 20)
		// 2. 窗口蒙层
		//CXLoading::AttachWnd(_句柄)
		//CXLoading::SetTheme(_句柄, UI工具主题_浅色)
		//CXLoading::SetStyle(_句柄, 加载样式_跳点)
		// 3. 自建独立元素
		//HELE hLoad = CXLoading::Create(100, 100, 120, 120, _布局元素1._句柄)
		//CXLoading::置主题(hLoad, UI工具主题_自动)
		//CXLoading::SetStyle(hLoad, 加载样式_脉冲)
		//CXLoading::SetCornerRadiusEx(hLoad, 12, 12, 0, 0)   // 上圆下方
		//CXLoading::SetSpeed(hLoad, 1.5f)
		// 4. 实时改文本 (小贴士轮换)
		//CXLoading::SetText(hLoad, "已加载 50%")
		//		CXLoading::SetText(hLoad, "已加载 90%, 即将完成")
		// 5. 停止 + 让出
		//CXLoading::Stop(hLoad)   // 元素恢复原内容
		// 6. 切风格 (相位重置, 不会瞬间跳)
		//CXLoading::SetStyle(hLoad, xloading_style_pulse)
		//CXLoading::Start(hLoad)
		占位

	函数 颜色选择器()
		CXColorPicker::SetBindEle(_按钮5._句柄, 0, 6)
		CXColorPicker::SetEnableAutoClose(FALSE)    // 失焦不关闭
		CXColorPicker::SetEnableModal(FALSE)        // 不阻塞父窗口
		CXColorPicker::SetEnableDrag(TRUE)            // 可拖动
		CXColorPicker::SetEnableTopmost(TRUE)       // 置顶

		// 恢复默认
		//CXColorPicker::SetEnableAutoClose(TRUE)
		//CXColorPicker::SetEnableModal(TRUE)
		//CXColorPicker::SetEnableDrag(FALSE)
		//CXColorPicker::SetEnableTopmost(FALSE)
		占位


	函数 整型 运行()
		blur.初始化(_句柄)
		//sd.附加窗口(_句柄)
		//sd.置主题(UI工具主题_深色)
		颜色选择器()
		//Test()
		//炫彩步骤条类测试()
		//折叠面板测试()
		//卡片面板测试()

		//布局大小类型_比例
		//布局大小类型_填充父
		//布局大小类型_自动
		//元素_调整布局()

		显示(真)
		返回 0

	函数 UI事件 整型 按钮点击_按钮2(元素句柄 来源句柄, 逻辑型* 是否拦截)
		月历日期时间 date, start, end

		CXCalendarCard::置绑定元素(_按钮2._句柄)
		CXCalendarCard::弹出单月历(_句柄, &date, FALSE, UI工具主题_自动)

		返回 0

	函数 UI事件 整型 按钮点击_显示双日期(元素句柄 来源句柄, 逻辑型* 是否拦截)
		月历日期时间 date, start, end
		CXCalendarCard::置绑定元素(_显示双日期._句柄)
		CXCalendarCard::弹出双月历(_句柄, &start, &end, TRUE, UI工具主题_自动)
		返回 0

	函数 UI事件 整型 按钮点击_按钮5(元素句柄 来源句柄, 逻辑型* 是否拦截)
		//steps.置当前步骤(3)
		颜色RGBA rgba{0}
		如果 (color.弹出颜色选择器(_句柄, &rgba, 假))
			调试输出(__函数名__, "颜色已经更新")

		返回 0


主窗口类  主窗口
函数 整型 入口函数_窗口()
	炫彩_初始化(真)
	炫彩_启用自动重绘UI(真)
	炫彩_置绘制频率(8)
	炫彩_启用DPI(真)
	炫彩_启用自动DPI(真)
	#加载资源文件
	主窗口.运行()
	炫彩_运行()
	炫彩_退出()
	返回 0
	


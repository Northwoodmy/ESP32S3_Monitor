# UI项目说明文件

## 项目简介
基于LVGL的UI界面项目，支持多屏幕显示和交互功能。

## 更新日志

### 版本 1.1.0 (2025-01-23)
- **重大更新**：将所有UI组件从"ui"前缀重命名为"ui2"前缀
- 重命名的文件包括：
  - 主要文件：ui.c/h -> ui2.c/h
  - 辅助文件：ui_helpers.c/h -> ui2_helpers.c/h
  - 事件文件：ui_events.h -> ui2_events.h  
  - 组件钩子：ui_comp_hook.c -> ui2_comp_hook.c
  - 所有屏幕文件：ui_*SCREEN.c/h -> ui2_*SCREEN.c/h
  - 字体文件：ui_font_*.c -> ui2_font_*.c
  - 图片文件：ui_img_*.c -> ui2_img_*.c
- **完整内部替换**：
  - 所有变量名：ui_* -> ui2_*
  - 所有函数名：_ui_* -> _ui2_*
  - 所有宏定义：_UI_* -> _UI2_*, UI_* -> UI2_*
  - 所有类型定义：ui_* -> ui2_*
  - 头文件保护符更新
- 更新了CMakeLists.txt构建配置
- 更新了filelist.txt文件列表
- 库名称从"ui"更改为"ui2"
- **区分大小写替换**：严格按照大小写规则进行替换

## 文件结构
- `ui2.c/h` - 主要UI初始化和管理
- `ui2_helpers.c/h` - UI辅助函数
- `ui2_events.h` - 事件定义
- `ui2_*SCREEN.c/h` - 各个屏幕的实现
- `ui2_font_*.c` - 字体文件
- `ui2_img_*.c` - 图片资源文件

## 使用说明
调用`ui2_init()`初始化UI系统，调用`ui2_destroy()`清理资源。 
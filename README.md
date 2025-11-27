# ChipTanks

STM32F103 上的 128×64 OLED 迷你射击游戏。C/C++ 混编，FreeRTOS 驱动，使用 ETL（Embedded Template Library）以适配嵌入式资源约束。

## 特性概览
- Roguelike 构筑：关卡/波次推进，中场与过关选卡，增益直接作用玩家数据。
- 实体系统：角色与子弹分层，统一管理器调度（行动 → 状态 → 清理 → 渲染）。
- 计时与节奏：用统一步长控制冷却、热量、回血、过场窗口，确定性强。
- 资源友好：固定容量容器（etl::vector），全局 new/delete 重定向至 FreeRTOS 堆。

## 目录结构（摘）
- App/
  - gameEntityManager.hpp（实体管理器）
  - gamePerkCardManager.hpp（选卡/增益管理器）
  - gameProgressManager.hpp（关卡/波次/Boss/过场）
  - threads.cpp（渲染/输入/控制三线程）
  - Bullet/（子弹抽象与实现）
  - Role/（玩家/敌人/基类实现）
  - Data/basicData.hpp（核心数据结构）
- Core/（HAL、FreeRTOS glue 等）
- Drivers/（STM32 HAL、CMSIS）
- Lib/etl/（Embedded Template Library）
- Middlewares/Third_Party/FreeRTOS/
- cmake/（ARM 工具链文件等）

## 构建
前置：
- ARM 交叉工具链（arm-none-eabi-gcc 系）
- CMake（建议 3.20+）与 Ninja（可选）

示例（PowerShell）：
```powershell
# Release 构建
cmake -S . -B build/Release -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release

# Debug 构建
cmake -S . -B build/Debug -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake `
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```
如使用 IDE/调试器：仓库根目录含有若干 .jdebug 文件，可按本地调试环境导入使用。

## 烧录与运行
- 烧录方式取决于本地工具链（如 J-Link、OpenOCD、IDE 内置烧录）。
- 上电后，显示开场动画 → 进入第一关。玩家存活则推进波次/关卡；清场时触发选卡或进入 Boss/通关流程。

## 运行机制
- 线程
  - 渲染线程：根据状态绘制开场/通关/Boss/选卡/战斗界面，控制刷新节奏。
  - 输入线程：按键扫描；战斗态写入玩家移动；选卡态移动光标并确认选择。
  - 控制线程：推进进度（清场→波次/关卡/选卡），调度实体行动/状态，清理无效对象。
- 管理器
  - 实体管理器：
    - addRole/addBullet，updateAllRolesActions/State，updateAllBulletsActions/State
    - 碰撞（AABB）与范围伤害，经验结算与资源回收，drawAllRoles/Bullets
  - 进度管理器：
    - 清场判定（场上仅玩家）→ 波次/关卡推进 → 敌人阵型或 Boss 生成 → 过场与展示
  - 卡片管理器：
    - 初始化仓库、随机抽取、应用所选、未选回库、绘制选卡界面

## 玩法与数值（摘）
- 子弹类型：
  - 普通弹：基础直击伤害。
  - 火球弹：直击后附带圆形范围溅射。
  - 闪电链：穿透，按伤害倍率与常数加成计算。
- 计时器：
  - 射击冷却：归零前禁射；步长与速度决定射速。
  - 热量系统：每次开火升温，周期降温；过热禁射。
  - 回血：累计到阈值离散恢复，直至上限。

## 开发要点
- FreeRTOS 头文件顺序：需先包含 FreeRTOS.h，再包含 task.h（管理器中已按此顺序）。
- 全局 new/delete 重载：见 App/reload_new_delete.cpp（路由至 pvPortMalloc/vPortFree）。
- 容量上限：角色最多 20，子弹最多 100；add 失败时需释放临时对象。
- 清场判定：当前以“容器仅剩玩家”作为清场条件；如引入友军/召唤物，建议改为“无敌方单位”。
- 子弹尺寸：构造子弹时需确保图片与尺寸有效，避免空指针读取尺寸。

## 致谢
- STM32 HAL/FreeRTOS、ETL 以及开源社区相关资源。
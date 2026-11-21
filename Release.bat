REM 双击运行产生Release目录，所需文件均在该目录

@echo off
chcp 65001 >nul

echo 正在设置环境...

REM 检查并创建/清理Release目录
if exist "Release\" (
    echo 清理Release目录...
    rmdir /s /q "Release"
)
echo 创建Release目录...
mkdir "Release"

REM 拷贝Scalpel.exe
if exist "build\Scalpel.exe" (
    echo 拷贝Scalpel.exe...
    copy "build\Scalpel.exe" "Release\"
) else (
    echo 错误: build\Scalpel.exe 文件不存在！
)

REM 切换到Release目录执行windeployqt
cd "Release"

REM 检查windeployqt.exe是否存在并执行
if exist "C:\Qt\6.12.0\mingw_64\bin\windeployqt.exe" (
    echo 执行windeployqt...
    "C:\Qt\6.12.0\mingw_64\bin\windeployqt.exe " --no-translations Scalpel.exe
) else (
    echo 错误: C:\Qt\6.12.0\mingw_64\bin\windeployqt.exe 文件不存在！
)

echo.
echo 所有操作完成！
echo.

REM 保持窗口打开
pause
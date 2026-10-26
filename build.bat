@echo off
setlocal
pushd "%~dp0"

set "MSXGL_ROOT=%CD%\MSXgl"
if not exist "%MSXGL_ROOT%\engine\script\js\build.js" (
	echo MSXgl build script not found: "%MSXGL_ROOT%\engine\script\js\build.js"
	popd
	exit /b 1
)

if exist "%MSXGL_ROOT%\tools\build\Node\node.exe" (
	"%MSXGL_ROOT%\tools\build\Node\node.exe" "%MSXGL_ROOT%\engine\script\js\build.js" %*
) else (
	node "%MSXGL_ROOT%\engine\script\js\build.js" %*
)
set "BUILD_EXIT=%ERRORLEVEL%"

popd
exit /b %BUILD_EXIT%

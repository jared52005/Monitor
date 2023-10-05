@echo off
echo "Building and debugging STM32F4xx target"
cd "APP_STM32F4xx\MDK-ARM"
"C:\Keil_v5\UV4\UV4.exe" -d APP_STM32F4xx.uvproj -t "BOARD_STM32F417_A" -o "build_log.txt"
if %ERRORLEVEL% GTR 1 (
echo [91mERROR
type "build_log.txt"
exit
)

echo [0m
@echo off
chcp 65001 >nul
cd /d "%~dp0"

set "EXE=.\CodeFabInterpreter.exe"

cls
echo.
echo  ============================================================
echo   CodeFabInterpreter  --  Demo
echo  ============================================================
echo.
echo   1  Basic / String / Variable / Scope / If / For  (FileRunner)
echo   2  Function + Recursion  (factorial, fib)
echo   3  Array
echo   4  Error Handling  (division by zero)
echo   5  Debug Mode  (step / break / watch)
echo.
pause


:: =============================================================
:: [1]  FileRunner -- 기본 연산 / 문자열 / 변수 / 조건문 / for
:: =============================================================
cls
echo.
echo  ============================================================
echo   [1]  FileRunner Mode
echo        Basic / String / Variable+Scope / If / For
echo  ============================================================
echo.
echo  ^> %EXE% run specDocument\testScript_positiveCase_withoutComment.txt
echo.
pause
echo.
%EXE% run specDocument\testScript_positiveCase_withoutComment.txt
echo.
pause


:: =============================================================
:: [2]  Function + Recursion
:: =============================================================
cls
echo.
echo  ============================================================
echo   [2]  Function + Recursion
echo        factorial(5) = 120  /  fib(8) = 21
echo  ============================================================
echo.
echo  func factorial(n) ^{ if (n ^< 2) ^{ return 1; ^} return n * factorial(n-1); ^}
echo  func fib(n)       ^{ if (n ^< 2) ^{ return n; ^} return fib(n-1)+fib(n-2); ^}
echo.
echo  ^> %EXE% run specDocument\demo_func.cb
echo.
pause
echo.
%EXE% run specDocument\demo_func.cb
echo.
pause


:: =============================================================
:: [3]  Array
:: =============================================================
cls
echo.
echo  ============================================================
echo   [3]  Array  --  Array(5), index access, sum = 150
echo  ============================================================
echo.
echo  var arr = Array(5);  arr[0]=10 ... arr[4]=50;
echo  for loop sum  --^>  150
echo.
echo  ^> %EXE% run specDocument\demo_array.cb
echo.
pause
echo.
%EXE% run specDocument\demo_array.cb
echo.
pause


:: =============================================================
:: [4]  Error Handling
:: =============================================================
cls
echo.
echo  ============================================================
echo   [4]  Error Handling  --  Division by zero
echo  ============================================================
echo.
echo  print 10 / 0;   --^>  [Error] 0 division
echo.
echo  ^> %EXE% run specDocument\demo_error.cb
echo.
pause
echo.
%EXE% run specDocument\demo_error.cb
echo.
pause


:: =============================================================
:: [5]  Debug Mode  (interactive)
:: =============================================================
cls
echo.
echo  ============================================================
echo   [5]  Debug Mode  --  Interactive
echo  ============================================================
echo.
echo   Suggested sequence:
echo     break 3       -- set breakpoint at line 3
echo     watch i       -- watch variable i
echo     watch result  -- watch variable result
echo     continue      -- run to breakpoint
echo     breakpoints   -- list all breakpoints
echo     step          -- step through
echo     inspect       -- print all variables in scope
echo     unwatch i     -- remove watch on i
echo     remove 3      -- remove breakpoint at line 3
echo.
echo  ^> %EXE% debug specDocument\demo_debug.cb
echo.
pause
echo.
%EXE% debug specDocument\demo_debug.cb
echo.

echo.
echo  ============================================================
echo   Demo Complete
echo  ============================================================
echo.
pause

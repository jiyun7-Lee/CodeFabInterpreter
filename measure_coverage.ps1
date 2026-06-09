
# measure_coverage.ps1
# Coverage build with clang-cl + llvm-cov (MSVC vcxproj unchanged)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---- paths ---------------------------------------------------------------
$llvmBin   = "C:\Program Files\LLVM\bin"
$clangCl   = "$llvmBin\clang-cl.exe"
$llvmProf  = "$llvmBin\llvm-profdata.exe"
$llvmCov   = "$llvmBin\llvm-cov.exe"

$root      = $PSScriptRoot
$src       = "$root\CodeFabInterpreter"
$gtestBase = "$root\packages\gmock.1.11.0\lib\native"
$outDir    = "$root\coverage_clang"
$exe       = "$outDir\test_coverage.exe"
$profRaw   = "$outDir\coverage.profraw"
$profData  = "$outDir\coverage.profdata"

foreach ($tool in @($clangCl, $llvmProf, $llvmCov)) {
    if (-not (Test-Path $tool)) { Write-Error "Tool not found: $tool" }
}
New-Item -ItemType Directory -Force $outDir | Out-Null

# ---- source list (mirrors vcxproj) ---------------------------------------
$prodSrcs = @(
    "Checker.cpp", "DebugController.cpp", "Environment.cpp",
    "Executor.cpp", "main.cpp", "Parser.cpp", "Shell.cpp", "Tokenizer.cpp"
) | ForEach-Object { "$src\$_" }

$testSrcs = @(
    "ArrayTest.cpp", "BreakpointTest.cpp", "CheckerTest.cpp",
    "DebugControllerTest.cpp", "DebugInfraTest.cpp", "ExecutorTest.cpp",
    "ExpressionParserTest.cpp", "FactoryShellTest.cpp", "FileModeTest.cpp",
    "FunctionTest.cpp", "integrationTest.cpp", "negativeTest.cpp",
    "OptimizationTest.cpp", "ParserTest.cpp", "ShellTest.cpp",
    "StepTest.cpp", "TokenizerTest.cpp", "WatchTest.cpp"
) | ForEach-Object { "$src\tests\$_" }

$gtestSrcs = @(
    "$gtestBase\src\gtest\gtest-all.cc",
    "$gtestBase\src\gmock\gmock-all.cc"
)

$allSrcs = $prodSrcs + $testSrcs + $gtestSrcs

# ---- [1/4] build ---------------------------------------------------------
Write-Host ""
Write-Host "[1/4] Building with clang-cl + coverage instrumentation..." -ForegroundColor Cyan

$buildArgs = @(
    "/std:c++20", "/utf-8", "/EHsc",
    "/MDd", "/DWIN32", "/D_CONSOLE",
    "/W0",
    "-fprofile-instr-generate",
    "-fcoverage-mapping",
    "-I$src",
    "-I$gtestBase\include",
    "-I$gtestBase\src\gtest",
    "-I$gtestBase\src\gmock"
) + $allSrcs + @("/link", "/out:$exe")

& $clangCl @buildArgs
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed (exit $LASTEXITCODE)" }
Write-Host "Build OK: $exe" -ForegroundColor Green

# ---- [2/4] run tests -----------------------------------------------------
Write-Host ""
Write-Host "[2/4] Running tests..." -ForegroundColor Cyan
$env:LLVM_PROFILE_FILE = $profRaw
& $exe --gtest_brief=1
if ($LASTEXITCODE -ne 0) { Write-Error "Tests failed (exit $LASTEXITCODE)" }

# ---- [3/4] merge profile data --------------------------------------------
Write-Host ""
Write-Host "[3/4] Merging profile data..." -ForegroundColor Cyan
& $llvmProf merge -sparse $profRaw -o $profData
if ($LASTEXITCODE -ne 0) { Write-Error "llvm-profdata failed" }

# ---- [4/4] report --------------------------------------------------------
Write-Host ""
Write-Host "[4/4] Coverage report" -ForegroundColor Cyan
Write-Host ("=" * 90)

& $llvmCov report $exe `
    "-instr-profile=$profData" `
    "-ignore-filename-regex=tests[/\\\\]" `
    "-ignore-filename-regex=gtest|gmock" `
    "-ignore-filename-regex=packages[/\\\\]"

Write-Host ("=" * 90)

# ---- HTML report ---------------------------------------------------------
$htmlDir = "$outDir\html"
New-Item -ItemType Directory -Force $htmlDir | Out-Null

& $llvmCov show $exe `
    "-instr-profile=$profData" `
    "-ignore-filename-regex=tests[/\\\\]" `
    "-ignore-filename-regex=gtest|gmock" `
    "-ignore-filename-regex=packages[/\\\\]" `
    "-format=html" `
    "-output-dir=$htmlDir"

Write-Host ""
Write-Host "HTML report: $htmlDir\index.html" -ForegroundColor Green

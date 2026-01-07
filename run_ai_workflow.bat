@echo off
REM AI-Powered Code Improvement Workflow Runner
REM Quick launcher for the AI workflow system

echo ========================================
echo AI-Powered Code Improvement Workflow
echo ========================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Please install Python 3.7+
    pause
    exit /b 1
)

REM Check if Ollama is available
ollama list >nul 2>&1
if errorlevel 1 (
    echo ERROR: Ollama not found or not running
    echo Please install Ollama and run: ollama serve
    pause
    exit /b 1
)

echo Choose an option:
echo [1] Full workflow - Dry run (safe, no changes)
echo [2] Full workflow - Apply changes (WARNING: modifies code)
echo [3] Analyze code only
echo [4] Generate tests only
echo [5] Extract configs only (dry run)
echo.

set /p choice="Enter choice (1-5): "

if "%choice%"=="1" (
    echo Running full workflow in DRY RUN mode...
    python .ai-tools\ai_workflow.py
)
if "%choice%"=="2" (
    echo.
    echo WARNING: This will modify your source code!
    set /p confirm="Are you sure? Type YES to continue: "
    if "!confirm!"=="YES" (
        echo Running full workflow with changes applied...
        python .ai-tools\ai_workflow.py --apply
    ) else (
        echo Cancelled.
    )
)
if "%choice%"=="3" (
    echo Running code analysis...
    python .ai-tools\code_analyzer.py --source-dir src --output-dir .ai-tools\analysis
)
if "%choice%"=="4" (
    echo Running test generator...
    python .ai-tools\test_generator.py --source-dir src --tests-dir tests
)
if "%choice%"=="5" (
    echo Running config extractor (dry run)...
    if exist .ai-tools\analysis\magic_numbers.json (
        python .ai-tools\config_extractor.py --magic-numbers .ai-tools\analysis\magic_numbers.json
    ) else (
        echo ERROR: No magic_numbers.json found. Run analysis first.
    )
)

echo.
echo ========================================
echo Workflow complete!
echo Check .ai-tools\ directory for results
echo ========================================
pause

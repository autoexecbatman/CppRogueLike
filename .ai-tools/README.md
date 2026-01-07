# AI-Powered Code Improvement Tools

This directory contains AI-powered tools for automated code analysis, refactoring, and test generation using local AI models (Ollama).

## Overview

The AI workflow system provides:
1. **Magic Number Detection** - Identifies hardcoded values that should be in configs
2. **Data/Behavior Separation** - Extracts configuration to JSON files
3. **Automated Test Generation** - Creates Google Test unit tests using AI analysis
4. **Code Quality Improvement** - Continuous automated improvements

## Prerequisites

- Python 3.7+
- Ollama running locally
- deepseek-coder-v2:16b-lite-instruct-q4_K_M model installed

```bash
# Install Ollama model
ollama pull deepseek-coder-v2:16b-lite-instruct-q4_K_M
```

## Tools

### 1. Code Analyzer (`code_analyzer.py`)

Scans C++ source code for improvement opportunities.

```bash
python .ai-tools/code_analyzer.py --source-dir src --output-dir .ai-tools/analysis
```

**Output:**
- `magic_numbers.json` - All detected hardcoded numbers
- `suggested_config.json` - AI-suggested configuration structure
- `test_suggestions.json` - Functions needing unit tests

### 2. Config Extractor (`config_extractor.py`)

Extracts magic numbers to JSON configuration files and generates a config loader class.

```bash
# Dry run (preview changes)
python .ai-tools/config_extractor.py --magic-numbers .ai-tools/analysis/magic_numbers.json

# Apply changes
python .ai-tools/config_extractor.py --magic-numbers .ai-tools/analysis/magic_numbers.json --apply
```

**Output:**
- `game_config.json` - Configuration values
- `GameConfig.h` - C++ config loader class
- `refactoring_plan.json` - Detailed refactoring steps

### 3. Test Generator (`test_generator.py`)

Generates Google Test unit tests using AI analysis.

```bash
# Generate from AI suggestions
python .ai-tools/test_generator.py --suggestions .ai-tools/analysis/test_suggestions.json --tests-dir tests

# Auto-generate for all untested files
python .ai-tools/test_generator.py --source-dir src --tests-dir tests
```

**Output:**
- Generated test files in `tests/` directory
- `generated_tests_manifest.json` - List of generated tests

### 4. AI Workflow Orchestrator (`ai_workflow.py`)

Runs the complete improvement workflow.

```bash
# Full workflow - dry run (safe, no changes)
python .ai-tools/ai_workflow.py

# Full workflow - apply changes
python .ai-tools/ai_workflow.py --apply

# Run specific steps
python .ai-tools/ai_workflow.py --step analyze
python .ai-tools/ai_workflow.py --step extract --apply
python .ai-tools/ai_workflow.py --step tests
```

## Quick Start

### Option 1: Quick Run (Batch Script)

```bash
# Windows
run_ai_workflow.bat

# Linux/Mac
./run_ai_workflow.sh
```

### Option 2: Manual Steps

```bash
# 1. Analyze code
python .ai-tools/code_analyzer.py --source-dir src

# 2. Review analysis results
cat .ai-tools/analysis/magic_numbers.json
cat .ai-tools/analysis/test_suggestions.json

# 3. Extract configs (dry run first)
python .ai-tools/config_extractor.py --magic-numbers .ai-tools/analysis/magic_numbers.json

# 4. Generate tests
python .ai-tools/test_generator.py --suggestions .ai-tools/analysis/test_suggestions.json

# 5. Review and apply changes if satisfied
python .ai-tools/config_extractor.py --magic-numbers .ai-tools/analysis/magic_numbers.json --apply
```

## Workflow Integration

### Local Automation

Run the workflow periodically using Task Scheduler (Windows) or cron (Linux):

**Windows Task Scheduler:**
```powershell
# Create a scheduled task
schtasks /create /tn "AI Code Improvement" /tr "python E:\repo\C++RogueLike\.ai-tools\ai_workflow.py" /sc daily /st 02:00
```

**Linux/Mac cron:**
```bash
# Add to crontab (runs daily at 2 AM)
0 2 * * * cd /path/to/C++RogueLike && python .ai-tools/ai_workflow.py
```

### GitHub Actions Integration

The tools can be integrated with GitHub Actions for automated improvements on commits. See `.github/workflows/ai-improvements.yml`.

## Safety Features

- **Dry Run by Default** - All tools default to dry run mode
- **Backup Recommendations** - Always commit before running with `--apply`
- **Incremental Changes** - Tools process files in batches
- **Review Required** - Generated code should be reviewed before merging

## Output Directory Structure

```
.ai-tools/
├── analysis/              # Code analysis results
│   ├── magic_numbers.json
│   ├── suggested_config.json
│   └── test_suggestions.json
├── configs/               # Extracted configurations
│   ├── game_config.json
│   ├── GameConfig.h
│   └── refactoring_plan.json
├── generated_tests/       # AI-generated tests
│   ├── *Test.cpp
│   └── generated_tests_manifest.json
└── workflow_report.txt    # Execution summary
```

## Best Practices

1. **Start with Analysis** - Run analyzer first to understand what will change
2. **Review AI Suggestions** - AI is helpful but not perfect
3. **Test After Changes** - Always run tests after applying refactoring
4. **Commit Frequently** - Commit before and after each major change
5. **Iterate** - Run the workflow multiple times for continuous improvement

## Troubleshooting

### Ollama Not Found
```bash
# Check if Ollama is running
ollama list

# Start Ollama service if needed
ollama serve
```

### Model Not Available
```bash
# Pull the required model
ollama pull deepseek-coder-v2:16b-lite-instruct-q4_K_M
```

### Python Dependencies
```bash
# No external dependencies required - uses stdlib only
# The scripts interact with Ollama via subprocess
```

## Examples

### Example 1: Find and Fix Magic Numbers

```bash
# Analyze
python .ai-tools/code_analyzer.py --source-dir src

# Review results
cat .ai-tools/analysis/magic_numbers.json | grep "health_potion"

# Extract to config
python .ai-tools/config_extractor.py --magic-numbers .ai-tools/analysis/magic_numbers.json --apply

# Now your code uses GameConfig::getInstance().getInt("ItemCreator", "HEALTH_POTION_VALUE", 50)
```

### Example 2: Generate Tests for New Class

```bash
# Generate tests for specific directory
python .ai-tools/test_generator.py --source-dir src/Factories --tests-dir tests/Factories

# Review generated tests
ls tests/Factories/*Test.cpp

# Add to CMake and run
cd build && make run_tests && ./tests/run_tests
```

## Contributing

To add new AI-powered tools:
1. Create a new Python script in `.ai-tools/`
2. Follow the pattern: analyze -> suggest -> apply
3. Always support dry run mode
4. Update `ai_workflow.py` to integrate your tool
5. Document in this README

## License

Same as main project.

# Ouroboros: Self-Healing Code Maintenance System

**Named after the serpent eating its own tail** - representing continuous self-improvement.

## Quick Start

### Prerequisites
- Python 3.8+
- Ollama running locally with deepseek-coder-v2 model
- CMake and C++ build tools

### Installation
```bash
# 1. Ensure Ollama is running
ollama serve

# 2. Verify model is installed
ollama list | grep deepseek-coder-v2

# 3. Test Ouroboros
cd .ai-tools
python ouroboros.py --mode scan
```

## Usage

### Dry Run (Safe, Recommended First)
```bash
python ouroboros.py --mode full
```
This will:
- Scan codebase for magic numbers
- Generate JSON config structure
- Show what would be changed
- **No files modified**

### Apply Changes
```bash
python ouroboros.py --mode full --apply
```
This will:
- Scan codebase
- Generate src/config/game_config.json
- Generate src/Config/Config.h and Config.cpp
- Refactor code to use `Config::category.member`
- Build and test
- Auto-rollback if build/tests fail
- Create branch for review

### Modes

**Scan Only**:
```bash
python ouroboros.py --mode scan
```
Output: `.ai-tools/output/health_report.json`

**Verify Current State**:
```bash
python ouroboros.py --mode verify
```
Tests current codebase without changes

**Show Last Report**:
```bash
python ouroboros.py --mode report
```
Displays last generated report

## What Ouroboros Does

### Before (Magic Numbers Everywhere)
```cpp
// src/Items/Items.cpp
pickable = std::make_unique<Healer>(10);
value = 50;

if (distance > 15) {
    attack();
}
```

### After (Data-Driven Config)

**game_config.json**:
```json
{
  "items": {
    "potions": {
      "health_potion_heal": 10,
      "health_potion_value": 50
    }
  },
  "ai": {
    "aggro_range": 15
  }
}
```

**Config.h** (auto-generated):
```cpp
struct ItemsConfig {
    int health_potion_heal;
    int health_potion_value;
};

class Config {
public:
    static ItemsConfig items;
    static void init();
};
```

**Refactored Code**:
```cpp
#include "Config/Config.h"

// Clean, self-documenting, data-driven
pickable = std::make_unique<Healer>(Config::items.health_potion_heal);
value = Config::items.health_potion_value;

if (distance > Config::ai.aggro_range) {
    attack();
}
```

## Benefits

✓ **No hardcoded values** - All config in JSON
✓ **Short syntax** - `Config::items.value` not verbose paths
✓ **Type-safe** - Compile-time struct members
✓ **No recompilation** - Edit JSON, restart game
✓ **Modding support** - Users can tweak values
✓ **Self-verifying** - Auto-builds and tests
✓ **Safe rollback** - Auto-reverts on failure

## Configuration

Edit `.ai-tools/ouroboros_config.json`:

```json
{
  "limits": {
    "batch_size": 10,           // Issues per run
    "max_function_length": 50   // Long function threshold
  },
  "safety": {
    "dry_run_default": true,    // Safe by default
    "require_tests": true,      // Must pass tests
    "auto_rollback": true       // Rollback on failure
  }
}
```

## Architecture

```
Ouroboros Pipeline:
┌─────────────────────────────────────┐
│ 1. SCAN (ouroboros_scanner.py)     │
│    Detects magic numbers, strings  │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│ 2. TRIAGE (ouroboros_triage.py)    │
│    Prioritizes and batches issues  │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│ 3. SURGERY (ouroboros_surgeon.py)  │
│    AI generates config + refactors │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│ 4. VALIDATE (ouroboros_validator.py)│
│    Builds and tests, rolls back    │
└──────────────┬──────────────────────┘
               ▼
┌─────────────────────────────────────┐
│ 5. PRESENT (ouroboros_presenter.py)│
│    Generates markdown report        │
└─────────────────────────────────────┘
```

## Output Files

After running, check `.ai-tools/output/`:
- `health_report.json` - Scan results
- `triage_queue.json` - Prioritized batches
- `surgery_results.json` - Applied changes
- `validation_report.json` - Build/test results
- `OUROBOROS_REPORT.md` - Human-readable summary

## Workflow Example

```bash
# 1. Run Ouroboros
python ouroboros.py --mode full --apply

# 2. Review report
cat .ai-tools/output/OUROBOROS_REPORT.md

# 3. Check the branch
git log ouroboros-refactor-YYYYMMDD_HHMMSS

# 4. Test manually (optional)
git checkout ouroboros-refactor-YYYYMMDD_HHMMSS
cmake --build build
build/bin/Debug/C++RogueLike.exe

# 5. Merge if satisfied
git checkout master
git merge ouroboros-refactor-YYYYMMDD_HHMMSS
git commit -m "Refactor: Extract magic numbers to JSON config (Ouroboros)"
```

## Troubleshooting

**"Failed to connect to Ollama"**:
```bash
# Start Ollama
ollama serve
```

**"Model not found"**:
```bash
# Pull the model
ollama pull deepseek-coder-v2:16b-lite-instruct-q4_K_M
```

**"Build failed"**:
- Ouroboros auto-rolls back on build failure
- Check `.ai-tools/output/validation_report.json` for errors
- Fix manually and re-run

**"Too many changes"**:
```bash
# Reduce batch size
python ouroboros.py --mode full --batch-size 5
```

## Future Enhancements

- [x] Magic number extraction to JSON config
- [x] Type-safe Config struct generation
- [x] Auto-build and test verification
- [ ] String literal centralization
- [ ] Long function extraction
- [ ] Duplicate code elimination
- [ ] const correctness fixes
- [ ] GitHub Actions integration

## Design Document

See `OUROBOROS_DESIGN.md` for full architectural details.

---

**Status**: Alpha - Core functionality complete, tested on C++ RogueLike codebase

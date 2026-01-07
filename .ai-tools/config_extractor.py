#!/usr/bin/env python3
"""
AI-Powered Config Extractor
Extracts magic numbers to JSON configs and refactors code
"""

import json
import re
import subprocess
from pathlib import Path
from typing import Dict, List, Any, Tuple


class ConfigExtractor:
    def __init__(self, ollama_model: str = "deepseek-coder-v2:16b-lite-instruct-q4_K_M"):
        self.ollama_model = ollama_model
        self.extracted_configs = {}

    def query_ollama(self, prompt: str) -> str:
        """Query local Ollama AI model."""
        try:
            result = subprocess.run(
                ['ollama', 'run', self.ollama_model, prompt],
                capture_output=True,
                text=True,
                timeout=120
            )
            return result.stdout.strip()
        except Exception as e:
            return f"Error: {e}"

    def analyze_magic_numbers(self, magic_numbers_file: str) -> Dict[str, Any]:
        """Analyze magic numbers and create config structure."""
        with open(magic_numbers_file, 'r') as f:
            magic_numbers = json.load(f)

        # Group by file and context
        grouped = {}
        for item in magic_numbers:
            file_name = Path(item['file']).stem
            if file_name not in grouped:
                grouped[file_name] = []
            grouped[file_name].append(item)

        return grouped

    def suggest_variable_name(self, context: str, number: str) -> str:
        """Use AI to suggest a meaningful variable name."""
        prompt = f"""Suggest a descriptive constant name for this magic number in C++ code.

Number: {number}
Context: {context}

Provide ONLY the constant name in UPPER_SNAKE_CASE format, no explanation.
Example: MAX_HEALTH_POTION_VALUE
"""

        response = self.query_ollama(prompt)
        # Clean up response
        name = response.strip().split('\n')[0].strip()
        # Remove any quotes or extra text
        name = re.sub(r'[^A-Z_0-9]', '', name.upper())

        if not name:
            name = f"CONFIG_VALUE_{number}"

        return name

    def generate_config_json(self, grouped_numbers: Dict, output_file: str):
        """Generate JSON configuration file."""
        config = {}

        for file_name, numbers in grouped_numbers.items():
            config[file_name] = {}

            for item in numbers[:20]:  # Limit per file
                var_name = self.suggest_variable_name(item['context'], item['number'])
                config[file_name][var_name] = int(item['number'])

        with open(output_file, 'w') as f:
            json.dump(config, f, indent=2)

        print(f"Generated config: {output_file}")
        return config

    def generate_config_loader(self, output_file: str):
        """Generate C++ config loader class."""
        code = '''#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <stdexcept>

using json = nlohmann::json;

class GameConfig {
private:
    json config_data;
    static GameConfig* instance;

    GameConfig() {
        loadConfig("game_config.json");
    }

public:
    static GameConfig& getInstance() {
        if (!instance) {
            instance = new GameConfig();
        }
        return *instance;
    }

    void loadConfig(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + filename);
        }
        file >> config_data;
    }

    template<typename T>
    T get(const std::string& category, const std::string& key, T default_value = T()) {
        try {
            if (config_data.contains(category) && config_data[category].contains(key)) {
                return config_data[category][key].get<T>();
            }
        } catch (...) {
            // Return default on any error
        }
        return default_value;
    }

    int getInt(const std::string& category, const std::string& key, int default_value = 0) {
        return get<int>(category, key, default_value);
    }

    double getDouble(const std::string& category, const std::string& key, double default_value = 0.0) {
        return get<double>(category, key, default_value);
    }

    std::string getString(const std::string& category, const std::string& key, const std::string& default_value = "") {
        return get<std::string>(category, key, default_value);
    }

    bool getBool(const std::string& category, const std::string& key, bool default_value = false) {
        return get<bool>(category, key, default_value);
    }
};

GameConfig* GameConfig::instance = nullptr;
'''

        with open(output_file, 'w') as f:
            f.write(code)

        print(f"Generated config loader: {output_file}")

    def create_refactoring_plan(self, magic_numbers_file: str, config_file: str) -> List[Dict]:
        """Create a refactoring plan for replacing magic numbers."""
        with open(magic_numbers_file, 'r') as f:
            magic_numbers = json.load(f)

        with open(config_file, 'r') as f:
            config = json.load(f)

        refactoring_plan = []

        for item in magic_numbers:
            file_path = item['file']
            line_num = item['line']
            number = item['number']
            context = item['context']

            # Find config key for this number
            file_name = Path(file_path).stem
            if file_name in config:
                # Find matching key
                for key, value in config[file_name].items():
                    if str(value) == number:
                        refactoring_plan.append({
                            'file': file_path,
                            'line': line_num,
                            'old_value': number,
                            'new_code': f'GameConfig::getInstance().getInt("{file_name}", "{key}", {number})',
                            'config_key': f'{file_name}.{key}'
                        })
                        break

        return refactoring_plan

    def apply_refactoring(self, refactoring_plan: List[Dict], dry_run: bool = True):
        """Apply refactoring changes to source files."""
        changes_by_file = {}

        for change in refactoring_plan:
            file_path = change['file']
            if file_path not in changes_by_file:
                changes_by_file[file_path] = []
            changes_by_file[file_path].append(change)

        for file_path, changes in changes_by_file.items():
            print(f"\nRefactoring {file_path}:")

            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
            except Exception as e:
                print(f"  Error reading file: {e}")
                continue

            # Sort changes by line number (descending to avoid line shifting)
            changes.sort(key=lambda x: x['line'], reverse=True)

            for change in changes[:5]:  # Limit changes per file for safety
                line_idx = change['line'] - 1
                if 0 <= line_idx < len(lines):
                    old_line = lines[line_idx]
                    # Simple replacement (can be improved)
                    new_line = old_line.replace(change['old_value'], change['new_code'])

                    if dry_run:
                        print(f"  Line {change['line']}:")
                        print(f"    Old: {old_line.strip()}")
                        print(f"    New: {new_line.strip()}")
                    else:
                        lines[line_idx] = new_line

            if not dry_run:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.writelines(lines)
                print(f"  Applied {len(changes)} changes")

    def run_full_extraction(self, magic_numbers_file: str, output_dir: str, dry_run: bool = True):
        """Run complete config extraction process."""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        print("Step 1: Analyzing magic numbers...")
        grouped = self.analyze_magic_numbers(magic_numbers_file)

        print(f"\nStep 2: Generating config JSON...")
        config_file = output_path / 'game_config.json'
        config = self.generate_config_json(grouped, str(config_file))

        print(f"\nStep 3: Generating config loader class...")
        loader_file = output_path / 'GameConfig.h'
        self.generate_config_loader(str(loader_file))

        print(f"\nStep 4: Creating refactoring plan...")
        plan = self.create_refactoring_plan(magic_numbers_file, str(config_file))

        plan_file = output_path / 'refactoring_plan.json'
        with open(plan_file, 'w') as f:
            json.dump(plan, f, indent=2)

        print(f"\nStep 5: Applying refactoring {'(DRY RUN)' if dry_run else ''}...")
        self.apply_refactoring(plan, dry_run)

        print(f"\n{'='*60}")
        print("Config extraction complete!")
        print(f"- Config file: {config_file}")
        print(f"- Loader class: {loader_file}")
        print(f"- Refactoring plan: {plan_file}")
        print(f"- Total changes planned: {len(plan)}")

        if dry_run:
            print("\nThis was a DRY RUN. Use --apply to make actual changes.")


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Extract configs from magic numbers')
    parser.add_argument('--magic-numbers', required=True,
                       help='Path to magic_numbers.json')
    parser.add_argument('--output-dir', default='.ai-tools/configs',
                       help='Output directory')
    parser.add_argument('--apply', action='store_true',
                       help='Apply changes (default is dry run)')
    parser.add_argument('--model', default='deepseek-coder-v2:16b-lite-instruct-q4_K_M')

    args = parser.parse_args()

    extractor = ConfigExtractor(args.model)
    extractor.run_full_extraction(
        args.magic_numbers,
        args.output_dir,
        dry_run=not args.apply
    )


if __name__ == '__main__':
    main()

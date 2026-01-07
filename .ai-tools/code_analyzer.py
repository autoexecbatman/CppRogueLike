#!/usr/bin/env python3
"""
AI-Powered Code Analyzer for C++ RogueLike
Scans source code for:
- Magic numbers to extract to JSON configs
- Functions that need unit tests
- Data/behavior separation opportunities
"""

import os
import re
import json
import subprocess
from pathlib import Path
from typing import List, Dict, Any, Tuple
import argparse

class CodeAnalyzer:
    def __init__(self, source_dir: str, ollama_model: str = "deepseek-coder-v2:16b-lite-instruct-q4_K_M"):
        self.source_dir = Path(source_dir)
        self.ollama_model = ollama_model
        self.magic_numbers = {}
        self.functions_needing_tests = []
        self.data_behavior_issues = []

    def scan_cpp_files(self) -> List[Path]:
        """Find all C++ source files."""
        cpp_files = []
        for ext in ['*.cpp', '*.h', '*.hpp']:
            cpp_files.extend(self.source_dir.rglob(ext))
        return cpp_files

    def detect_magic_numbers(self, file_path: Path) -> List[Dict[str, Any]]:
        """Detect magic numbers in a C++ file."""
        magic_numbers = []

        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')

        # Regex patterns for magic numbers
        # Skip common valid numbers: 0, 1, -1, 2, 10, 100
        number_pattern = re.compile(r'\b(?!0\b|1\b|-1\b|2\b|10\b|100\b)(\d+)\b')

        for line_num, line in enumerate(lines, 1):
            # Skip comments
            if '//' in line:
                line = line[:line.index('//')]

            # Find numbers
            matches = number_pattern.finditer(line)
            for match in matches:
                number = match.group(1)
                context = line.strip()

                magic_numbers.append({
                    'file': str(file_path),
                    'line': line_num,
                    'number': number,
                    'context': context
                })

        return magic_numbers

    def query_ollama(self, prompt: str) -> str:
        """Query local Ollama AI model."""
        try:
            result = subprocess.run(
                ['ollama', 'run', self.ollama_model, prompt],
                capture_output=True,
                text=True,
                timeout=60
            )
            return result.stdout.strip()
        except Exception as e:
            return f"Error querying AI: {e}"

    def analyze_function_for_tests(self, file_path: Path, function_code: str, function_name: str) -> Dict[str, Any]:
        """Use AI to analyze if a function needs tests and suggest test cases."""
        prompt = f"""Analyze this C++ function and determine if it needs unit tests.
If yes, suggest 3-5 test cases covering edge cases, normal cases, and error cases.

Function: {function_name}
Code:
{function_code}

Respond in JSON format:
{{
    "needs_tests": true/false,
    "reason": "why it needs tests",
    "test_cases": [
        {{"name": "test_case_name", "description": "what it tests", "input": "input values", "expected": "expected result"}}
    ]
}}
"""
        response = self.query_ollama(prompt)
        try:
            # Try to extract JSON from response
            json_match = re.search(r'\{.*\}', response, re.DOTALL)
            if json_match:
                return json.loads(json_match.group(0))
        except:
            pass

        return {
            "needs_tests": False,
            "reason": "Could not analyze",
            "test_cases": []
        }

    def extract_functions(self, file_path: Path) -> List[Tuple[str, str]]:
        """Extract function definitions from C++ file."""
        functions = []

        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        # Simple regex for function detection (not perfect but works for many cases)
        # Matches: return_type function_name(params) {
        function_pattern = re.compile(
            r'(?:^|\n)(?:static\s+)?(?:inline\s+)?'
            r'(\w+(?:\s*\*)?)\s+'  # return type
            r'(\w+)\s*'  # function name
            r'\([^)]*\)\s*'  # parameters
            r'\{',  # opening brace
            re.MULTILINE
        )

        for match in function_pattern.finditer(content):
            return_type = match.group(1)
            func_name = match.group(2)

            # Extract function body (simplified - just get a reasonable chunk)
            start = match.start()
            # Get ~20 lines or until we find likely end
            end = content.find('\n}\n', start)
            if end == -1:
                end = start + 1000

            func_code = content[start:end+2]
            functions.append((func_name, func_code))

        return functions

    def suggest_config_structure(self, magic_numbers: List[Dict]) -> Dict[str, Any]:
        """Use AI to suggest how to organize magic numbers into configs."""
        if not magic_numbers:
            return {}

        # Group by file
        by_file = {}
        for item in magic_numbers[:20]:  # Limit for AI context
            file_name = Path(item['file']).name
            if file_name not in by_file:
                by_file[file_name] = []
            by_file[file_name].append(item)

        prompt = f"""Analyze these magic numbers from C++ code and suggest a JSON configuration structure.
Group related numbers into logical categories (e.g., game_balance, ui_settings, combat_stats).

Magic numbers found:
{json.dumps(by_file, indent=2)}

Respond with a suggested JSON config structure with meaningful category names and variable names.
Example format:
{{
    "game_balance": {{
        "health_potion_value": 50,
        "scroll_value": 150
    }},
    "ui_settings": {{
        "window_width": 800
    }}
}}
"""

        response = self.query_ollama(prompt)
        try:
            json_match = re.search(r'\{.*\}', response, re.DOTALL)
            if json_match:
                return json.loads(json_match.group(0))
        except:
            pass

        return {}

    def generate_report(self) -> str:
        """Generate analysis report."""
        report = []
        report.append("=" * 80)
        report.append("CODE ANALYSIS REPORT")
        report.append("=" * 80)
        report.append("")

        report.append(f"Total magic numbers found: {len(self.magic_numbers)}")
        report.append(f"Functions needing tests: {len(self.functions_needing_tests)}")
        report.append("")

        if self.magic_numbers:
            report.append("MAGIC NUMBERS DETECTED:")
            report.append("-" * 80)
            for category, numbers in list(self.magic_numbers.items())[:10]:
                report.append(f"\n{category}:")
                for num in numbers[:5]:
                    report.append(f"  Line {num['line']}: {num['context'][:60]}")

        report.append("")
        report.append("=" * 80)

        return "\n".join(report)

    def run_full_analysis(self, output_dir: str = ".ai-tools/analysis"):
        """Run complete code analysis."""
        print("Starting code analysis...")

        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        # Scan for C++ files
        cpp_files = self.scan_cpp_files()
        print(f"Found {len(cpp_files)} C++ files")

        # Detect magic numbers
        all_magic_numbers = []
        for cpp_file in cpp_files:
            if 'test' not in str(cpp_file).lower():  # Skip test files
                magic_nums = self.detect_magic_numbers(cpp_file)
                all_magic_numbers.extend(magic_nums)

        print(f"Detected {len(all_magic_numbers)} magic numbers")

        # Save magic numbers
        with open(output_path / 'magic_numbers.json', 'w') as f:
            json.dump(all_magic_numbers, f, indent=2)

        # Get AI suggestions for config structure
        print("Asking AI for config organization suggestions...")
        config_suggestion = self.suggest_config_structure(all_magic_numbers)

        with open(output_path / 'suggested_config.json', 'w') as f:
            json.dump(config_suggestion, f, indent=2)

        # Analyze functions for test coverage
        print("Analyzing functions for test coverage...")
        test_suggestions = []

        for cpp_file in cpp_files[:5]:  # Limit to first 5 files for demo
            if cpp_file.suffix == '.cpp' and 'test' not in str(cpp_file).lower():
                functions = self.extract_functions(cpp_file)

                for func_name, func_code in functions[:3]:  # Limit functions per file
                    print(f"  Analyzing {func_name}...")
                    analysis = self.analyze_function_for_tests(cpp_file, func_code, func_name)

                    if analysis.get('needs_tests'):
                        test_suggestions.append({
                            'file': str(cpp_file),
                            'function': func_name,
                            'analysis': analysis
                        })

        with open(output_path / 'test_suggestions.json', 'w') as f:
            json.dump(test_suggestions, f, indent=2)

        print(f"\nAnalysis complete! Results saved to {output_path}/")
        print(f"- magic_numbers.json: {len(all_magic_numbers)} magic numbers detected")
        print(f"- suggested_config.json: AI-suggested configuration structure")
        print(f"- test_suggestions.json: {len(test_suggestions)} functions needing tests")

        return output_path

def main():
    parser = argparse.ArgumentParser(description='Analyze C++ code for improvements')
    parser.add_argument('--source-dir', default='src', help='Source directory to analyze')
    parser.add_argument('--model', default='deepseek-coder-v2:16b-lite-instruct-q4_K_M',
                       help='Ollama model to use')
    parser.add_argument('--output-dir', default='.ai-tools/analysis',
                       help='Output directory for analysis results')

    args = parser.parse_args()

    analyzer = CodeAnalyzer(args.source_dir, args.model)
    analyzer.run_full_analysis(args.output_dir)

if __name__ == '__main__':
    main()

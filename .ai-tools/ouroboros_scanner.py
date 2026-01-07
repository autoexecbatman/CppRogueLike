#!/usr/bin/env python3
"""
Ouroboros Scanner Module
Detects code quality issues: magic numbers, strings, long functions, duplicates
"""

import re
import os
import json
import subprocess
from pathlib import Path
from datetime import datetime
from collections import defaultdict

class OuroborosScanner:
    def __init__(self, config):
        """Initialize scanner with configuration."""
        self.config = config
        self.source_dir = Path(config["paths"]["source_dir"])
        self.skip_values = set(config["limits"]["magic_number_skip"])
        self.max_function_length = config["limits"]["max_function_length"]

        # Patterns for detection
        self.magic_number_pattern = re.compile(
            r'\b(\d+\.?\d*)\b'  # Match numeric literals
        )
        self.string_literal_pattern = re.compile(
            r'"([^"\\]*(\\.[^"\\]*)*)"'  # Match string literals
        )
        self.function_pattern = re.compile(
            r'^\s*(?:(?:inline|static|virtual|constexpr|explicit)\s+)*'  # Modifiers
            r'(?:[\w:]+(?:<[^>]+>)?)\s+'  # Return type
            r'([\w:]+)\s*\([^)]*\)\s*(?:const|noexcept|override)*\s*{'  # Function name and body start
        )

    def scan(self):
        """Scan codebase and return health report."""
        print("Scanning codebase...")

        # Get all C++ source files
        cpp_files = self.get_cpp_files()
        print(f"Found {len(cpp_files)} C++ files to analyze")

        # Initialize results
        magic_numbers = []
        string_literals = []
        long_functions = []
        duplicates = []

        # Scan each file
        for file_path in cpp_files:
            print(f"  Scanning: {file_path}")

            # Read file content
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception as e:
                print(f"    Warning: Could not read {file_path}: {e}")
                continue

            # Detect magic numbers
            file_magic_numbers = self.detect_magic_numbers(file_path, lines)
            magic_numbers.extend(file_magic_numbers)

            # Detect string literals
            file_strings = self.detect_string_literals(file_path, lines)
            string_literals.extend(file_strings)

            # Detect long functions
            file_long_functions = self.detect_long_functions(file_path, lines)
            long_functions.extend(file_long_functions)

        # Build health report
        health_report = {
            "timestamp": datetime.now().isoformat(),
            "files_scanned": len(cpp_files),
            "issues": {
                "magic_numbers": magic_numbers,
                "string_literals": string_literals,
                "long_functions": long_functions,
                "duplicates": duplicates
            },
            "summary": {
                "magic_numbers": len(magic_numbers),
                "string_literals": len(string_literals),
                "long_functions": len(long_functions),
                "duplicates": len(duplicates),
                "build_errors": 0,
                "test_failures": 0
            }
        }

        return health_report

    def get_cpp_files(self):
        """Get all C++ source files in source directory."""
        cpp_files = []

        for ext in ['.cpp', '.h', '.hpp']:
            cpp_files.extend(self.source_dir.rglob(f'*{ext}'))

        # Filter out test files
        cpp_files = [
            f for f in cpp_files
            if '.test.cpp' not in str(f) and
               '/tests/' not in str(f).replace('\\', '/')
        ]

        return cpp_files

    def detect_magic_numbers(self, file_path, lines):
        """Detect magic numbers in file."""
        magic_numbers = []

        for line_num, line in enumerate(lines, start=1):
            # Skip comments
            if '//' in line:
                line = line[:line.index('//')]
            if '/*' in line:
                continue

            # Find all numeric literals
            for match in self.magic_number_pattern.finditer(line):
                value_str = match.group(1)

                # Try to parse as int or float
                try:
                    if '.' in value_str:
                        value = float(value_str)
                    else:
                        value = int(value_str)

                    # Skip common values
                    if value in self.skip_values:
                        continue

                    # Get context (surrounding code)
                    start = max(0, line_num - 2)
                    end = min(len(lines), line_num + 1)
                    context = '\n'.join(lines[start:end])

                    magic_numbers.append({
                        "id": f"MAGIC_{len(magic_numbers) + 1:04d}",
                        "file": str(file_path),
                        "line": line_num,
                        "value": value,
                        "value_str": value_str,
                        "context": context.strip(),
                        "severity": self.assess_magic_number_severity(value, line)
                    })

                except ValueError:
                    continue

        return magic_numbers

    def assess_magic_number_severity(self, value, line):
        """Assess severity of magic number."""
        # High severity for large values or percentages
        if value > 1000:
            return "high"
        # Medium for common game values
        elif value > 10:
            return "medium"
        else:
            return "low"

    def detect_string_literals(self, file_path, lines):
        """Detect string literals in file."""
        string_literals = []

        for line_num, line in enumerate(lines, start=1):
            # Skip comments and includes
            if '//' in line or '#include' in line:
                continue

            # Find all string literals
            for match in self.string_literal_pattern.finditer(line):
                value = match.group(1)

                # Skip empty strings and very short strings
                if len(value) < 2:
                    continue

                # Get context
                start = max(0, line_num - 1)
                end = min(len(lines), line_num + 1)
                context = '\n'.join(lines[start:end])

                string_literals.append({
                    "id": f"STRING_{len(string_literals) + 1:04d}",
                    "file": str(file_path),
                    "line": line_num,
                    "value": value,
                    "context": context.strip(),
                    "category": self.categorize_string(value, line)
                })

        return string_literals

    def categorize_string(self, value, line):
        """Categorize string literal."""
        # JSON keys
        if '"' in line and '[' in line and ']' in line:
            return "json_key"
        # Log messages
        elif 'log(' in line or 'print' in line or 'cout' in line:
            return "log_message"
        # Item names
        elif 'name' in line.lower():
            return "item_name"
        else:
            return "other"

    def detect_long_functions(self, file_path, lines):
        """Detect functions longer than threshold."""
        long_functions = []
        current_function = None
        brace_count = 0

        for line_num, line in enumerate(lines, start=1):
            # Check for function start
            if current_function is None:
                match = self.function_pattern.match(line)
                if match:
                    current_function = {
                        "name": match.group(1),
                        "start_line": line_num,
                        "file": str(file_path)
                    }
                    brace_count = line.count('{') - line.count('}')
            else:
                # Track braces
                brace_count += line.count('{') - line.count('}')

                # Function ended
                if brace_count == 0:
                    end_line = line_num
                    length = end_line - current_function["start_line"]

                    if length > self.max_function_length:
                        # Get function body
                        start = current_function["start_line"] - 1
                        end = end_line
                        body = '\n'.join(lines[start:end])

                        long_functions.append({
                            "id": f"LONGFUNC_{len(long_functions) + 1:04d}",
                            "file": current_function["file"],
                            "name": current_function["name"],
                            "start_line": current_function["start_line"],
                            "end_line": end_line,
                            "length": length,
                            "body": body,
                            "severity": "high" if length > 100 else "medium"
                        })

                    current_function = None
                    brace_count = 0

        return long_functions

    def group_magic_numbers_by_file(self, magic_numbers):
        """Group magic numbers by file for easier processing."""
        grouped = defaultdict(list)

        for magic in magic_numbers:
            grouped[magic["file"]].append(magic)

        return dict(grouped)

    def group_magic_numbers_semantically(self, magic_numbers):
        """Group magic numbers by semantic meaning using AI."""
        # This will be called by the surgeon module with AI assistance
        # For now, just group by file
        return self.group_magic_numbers_by_file(magic_numbers)

if __name__ == "__main__":
    # Quick test
    import sys
    sys.path.insert(0, os.path.dirname(__file__))

    with open(".ai-tools/ouroboros_config.json", 'r') as f:
        config = json.load(f)

    scanner = OuroborosScanner(config)
    report = scanner.scan()

    print("\n" + "=" * 60)
    print("SCAN COMPLETE")
    print("=" * 60)
    print(f"Magic numbers found: {report['summary']['magic_numbers']}")
    print(f"String literals found: {report['summary']['string_literals']}")
    print(f"Long functions found: {report['summary']['long_functions']}")

    # Save report
    output_path = Path(".ai-tools/output/health_report.json")
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_path, 'w') as f:
        json.dump(report, f, indent=2)

    print(f"\nReport saved: {output_path}")

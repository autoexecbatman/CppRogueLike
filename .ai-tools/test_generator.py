#!/usr/bin/env python3
"""
AI-Powered Test Generator
Generates Google Test files based on AI analysis
"""

import json
import subprocess
from pathlib import Path
from typing import Dict, List, Any


class TestGenerator:
    def __init__(self, ollama_model: str = "deepseek-coder-v2:16b-lite-instruct-q4_K_M"):
        self.ollama_model = ollama_model

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

    def read_source_file(self, file_path: str) -> str:
        """Read source file content."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                return f.read()
        except Exception as e:
            return f"Error reading file: {e}"

    def generate_test_for_class(self, source_file: str, class_name: str) -> str:
        """Generate Google Test code for a class."""
        source_content = self.read_source_file(source_file)

        prompt = f"""Generate comprehensive Google Test unit tests for this C++ class.

Source file: {source_file}
Class: {class_name}

Source code (first 2000 chars):
{source_content[:2000]}

Requirements:
1. Use Google Test framework (gtest/gtest.h)
2. Create a test fixture class inheriting from ::testing::Test
3. Include proper headers
4. Test normal cases, edge cases, and error conditions
5. Use EXPECT_EQ, EXPECT_NE, EXPECT_TRUE, EXPECT_FALSE, etc.
6. Follow the naming convention: ClassNameTest for fixture, TEST_F for tests
7. Include at least 5-7 different test cases

Generate ONLY the C++ test code, no explanations.
"""

        response = self.query_ollama(prompt)
        return response

    def generate_test_for_function(self, file_path: str, function_name: str,
                                   test_cases: List[Dict]) -> str:
        """Generate Google Test code for a specific function."""
        source_content = self.read_source_file(file_path)

        # Find the function in source
        function_code = ""
        lines = source_content.split('\n')
        for i, line in enumerate(lines):
            if function_name in line and '(' in line:
                # Extract function (simplified)
                start = i
                end = min(i + 50, len(lines))
                function_code = '\n'.join(lines[start:end])
                break

        test_cases_str = json.dumps(test_cases, indent=2)

        prompt = f"""Generate Google Test unit tests for this C++ function.

Function: {function_name}
File: {file_path}

Function code:
{function_code}

Suggested test cases:
{test_cases_str}

Requirements:
1. Use Google Test framework
2. Create TEST() or TEST_F() for each test case
3. Include proper headers and namespaces
4. Use appropriate EXPECT_* assertions
5. Cover all suggested test cases

Generate ONLY the C++ test code.
"""

        response = self.query_ollama(prompt)
        return response

    def process_test_suggestions(self, suggestions_file: str, output_dir: str):
        """Process AI test suggestions and generate test files."""
        with open(suggestions_file, 'r') as f:
            suggestions = json.load(f)

        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        generated_tests = []

        for suggestion in suggestions:
            file_path = suggestion['file']
            function_name = suggestion['function']
            analysis = suggestion['analysis']

            if not analysis.get('needs_tests'):
                continue

            print(f"Generating tests for {function_name}...")

            test_code = self.generate_test_for_function(
                file_path,
                function_name,
                analysis.get('test_cases', [])
            )

            # Determine output file name
            source_path = Path(file_path)
            test_file_name = f"{source_path.stem}_{function_name}_Test.cpp"
            output_file = output_path / test_file_name

            # Save generated test
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(test_code)

            generated_tests.append({
                'source': file_path,
                'function': function_name,
                'test_file': str(output_file)
            })

            print(f"  Generated: {output_file}")

        # Save manifest
        with open(output_path / 'generated_tests_manifest.json', 'w') as f:
            json.dump(generated_tests, f, indent=2)

        print(f"\nGenerated {len(generated_tests)} test files")
        return generated_tests

    def analyze_and_generate_for_directory(self, source_dir: str, tests_dir: str):
        """Analyze a directory and generate tests for untested files."""
        source_path = Path(source_dir)
        tests_path = Path(tests_dir)

        cpp_files = list(source_path.rglob('*.cpp'))
        print(f"Found {len(cpp_files)} C++ files in {source_dir}")

        for cpp_file in cpp_files:
            if 'test' in str(cpp_file).lower():
                continue

            # Check if test already exists
            relative_path = cpp_file.relative_to(source_path)
            test_file = tests_path / relative_path.parent / f"{cpp_file.stem}Test.cpp"

            if test_file.exists():
                print(f"Test exists for {cpp_file.name}, skipping")
                continue

            print(f"\nAnalyzing {cpp_file.name}...")

            # Extract class name from file name (simple heuristic)
            class_name = cpp_file.stem

            # Generate test
            test_code = self.generate_test_for_class(str(cpp_file), class_name)

            # Create test directory if needed
            test_file.parent.mkdir(parents=True, exist_ok=True)

            # Save test file
            with open(test_file, 'w', encoding='utf-8') as f:
                f.write(test_code)

            print(f"  Generated: {test_file}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Generate unit tests using AI')
    parser.add_argument('--suggestions', help='Path to test_suggestions.json')
    parser.add_argument('--source-dir', default='src', help='Source directory')
    parser.add_argument('--tests-dir', default='tests', help='Tests output directory')
    parser.add_argument('--model', default='deepseek-coder-v2:16b-lite-instruct-q4_K_M')

    args = parser.parse_args()

    generator = TestGenerator(args.model)

    if args.suggestions:
        generator.process_test_suggestions(args.suggestions, args.tests_dir)
    else:
        generator.analyze_and_generate_for_directory(args.source_dir, args.tests_dir)


if __name__ == '__main__':
    main()

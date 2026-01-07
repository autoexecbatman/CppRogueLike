#!/usr/bin/env python3
"""Quick test to verify AI tools are working"""

import subprocess
import sys
from pathlib import Path

def test_ollama():
    """Test if Ollama is accessible."""
    print("Testing Ollama...")
    try:
        result = subprocess.run(
            ['ollama', 'list'],
            capture_output=True,
            text=True,
            timeout=5
        )
        if 'deepseek-coder-v2' in result.stdout:
            print("[OK] Ollama is running with deepseek-coder-v2 model")
            return True
        else:
            print("[FAIL] deepseek-coder-v2 model not found")
            return False
    except Exception as e:
        print(f"[FAIL] Ollama not accessible: {e}")
        return False

def test_python_files():
    """Test if all Python files are present."""
    print("\nChecking Python scripts...")
    tools_dir = Path(__file__).parent
    required_files = [
        'code_analyzer.py',
        'test_generator.py',
        'config_extractor.py',
        'ai_workflow.py'
    ]

    all_present = True
    for file in required_files:
        file_path = tools_dir / file
        if file_path.exists():
            print(f"[OK] {file}")
        else:
            print(f"[FAIL] {file} - NOT FOUND")
            all_present = False

    return all_present

def test_simple_query():
    """Test a simple Ollama query."""
    print("\nTesting AI query...")
    try:
        result = subprocess.run(
            ['ollama', 'run', 'deepseek-coder-v2:16b-lite-instruct-q4_K_M',
             'Reply with only the word WORKING if you can read this'],
            capture_output=True,
            text=True,
            timeout=30
        )
        if 'WORKING' in result.stdout.upper():
            print("[OK] AI model responds correctly")
            return True
        else:
            print(f"[FAIL] Unexpected response: {result.stdout[:100]}")
            return False
    except Exception as e:
        print(f"[FAIL] Query failed: {e}")
        return False

def main():
    print("="*60)
    print("AI Tools Quick Test")
    print("="*60)

    tests = [
        ("Ollama Service", test_ollama),
        ("Python Scripts", test_python_files),
        ("AI Query", test_simple_query)
    ]

    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print(f"[FAIL] {name} failed with error: {e}")
            results.append((name, False))

    print("\n" + "="*60)
    print("Test Results Summary")
    print("="*60)

    all_passed = True
    for name, passed in results:
        status = "PASS" if passed else "FAIL"
        symbol = "[OK]" if passed else "[FAIL]"
        print(f"{symbol} {name}: {status}")
        if not passed:
            all_passed = False

    print("="*60)

    if all_passed:
        print("\nAll tests passed! AI tools are ready to use.")
        print("Run: python .ai-tools/ai_workflow.py")
        return 0
    else:
        print("\nSome tests failed. Please check the errors above.")
        return 1

if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""
AI-Powered Code Improvement Workflow
Orchestrates the complete code improvement process
"""

import subprocess
import argparse
from pathlib import Path
import json
import sys


class AIWorkflow:
    def __init__(self, dry_run: bool = True, verbose: bool = True):
        self.dry_run = dry_run
        self.verbose = verbose
        self.root_dir = Path(__file__).parent.parent
        self.tools_dir = self.root_dir / '.ai-tools'
        self.analysis_dir = self.tools_dir / 'analysis'
        self.configs_dir = self.tools_dir / 'configs'
        self.generated_tests_dir = self.tools_dir / 'generated_tests'

    def log(self, message: str):
        """Log message if verbose."""
        if self.verbose:
            print(message)

    def run_command(self, command: list, description: str = "") -> bool:
        """Run a command and return success status."""
        if description:
            self.log(f"\n{description}")

        self.log(f"Running: {' '.join(command)}")

        try:
            result = subprocess.run(
                command,
                cwd=str(self.root_dir),
                capture_output=True,
                text=True,
                timeout=300
            )

            if self.verbose:
                if result.stdout:
                    print(result.stdout)
                if result.stderr:
                    print("STDERR:", result.stderr)

            return result.returncode == 0

        except subprocess.TimeoutExpired:
            self.log("ERROR: Command timed out")
            return False
        except Exception as e:
            self.log(f"ERROR: {e}")
            return False

    def step_analyze_code(self) -> bool:
        """Step 1: Analyze codebase for magic numbers and test opportunities."""
        self.log("="*60)
        self.log("STEP 1: Analyzing codebase...")
        self.log("="*60)

        command = [
            'python',
            str(self.tools_dir / 'code_analyzer.py'),
            '--source-dir', 'src',
            '--output-dir', str(self.analysis_dir)
        ]

        return self.run_command(command, "Analyzing source code...")

    def step_extract_configs(self) -> bool:
        """Step 2: Extract magic numbers to JSON configs."""
        self.log("\n" + "="*60)
        self.log("STEP 2: Extracting configuration...")
        self.log("="*60)

        magic_numbers_file = self.analysis_dir / 'magic_numbers.json'
        if not magic_numbers_file.exists():
            self.log("ERROR: magic_numbers.json not found. Run analysis first.")
            return False

        command = [
            'python',
            str(self.tools_dir / 'config_extractor.py'),
            '--magic-numbers', str(magic_numbers_file),
            '--output-dir', str(self.configs_dir)
        ]

        if not self.dry_run:
            command.append('--apply')

        return self.run_command(command, "Extracting configs...")

    def step_generate_tests(self) -> bool:
        """Step 3: Generate unit tests based on AI analysis."""
        self.log("\n" + "="*60)
        self.log("STEP 3: Generating unit tests...")
        self.log("="*60)

        suggestions_file = self.analysis_dir / 'test_suggestions.json'
        if not suggestions_file.exists():
            self.log("WARNING: test_suggestions.json not found. Generating basic tests...")

            command = [
                'python',
                str(self.tools_dir / 'test_generator.py'),
                '--source-dir', 'src',
                '--tests-dir', 'tests'
            ]
        else:
            command = [
                'python',
                str(self.tools_dir / 'test_generator.py'),
                '--suggestions', str(suggestions_file),
                '--tests-dir', str(self.generated_tests_dir)
            ]

        return self.run_command(command, "Generating tests...")

    def step_create_report(self):
        """Step 4: Create summary report."""
        self.log("\n" + "="*60)
        self.log("STEP 4: Creating summary report...")
        self.log("="*60)

        report = []
        report.append("AI WORKFLOW EXECUTION REPORT")
        report.append("="*60)

        # Check analysis results
        magic_numbers_file = self.analysis_dir / 'magic_numbers.json'
        if magic_numbers_file.exists():
            with open(magic_numbers_file) as f:
                magic_nums = json.load(f)
            report.append(f"\nMagic Numbers Detected: {len(magic_nums)}")

        # Check config extraction
        config_file = self.configs_dir / 'game_config.json'
        if config_file.exists():
            with open(config_file) as f:
                configs = json.load(f)
            total_configs = sum(len(v) for v in configs.values())
            report.append(f"Configuration Entries Created: {total_configs}")

        # Check test generation
        test_manifest = self.generated_tests_dir / 'generated_tests_manifest.json'
        if test_manifest.exists():
            with open(test_manifest) as f:
                tests = json.load(f)
            report.append(f"Test Files Generated: {len(tests)}")

        report.append("\n" + "="*60)
        report.append(f"Mode: {'DRY RUN' if self.dry_run else 'APPLIED CHANGES'}")

        report_text = "\n".join(report)
        self.log(report_text)

        # Save report
        report_file = self.tools_dir / 'workflow_report.txt'
        with open(report_file, 'w') as f:
            f.write(report_text)

        self.log(f"\nReport saved to: {report_file}")

    def run_full_workflow(self) -> bool:
        """Run the complete AI improvement workflow."""
        self.log("Starting AI-Powered Code Improvement Workflow")
        self.log(f"Root directory: {self.root_dir}")
        self.log(f"Dry run mode: {self.dry_run}")

        # Create necessary directories
        self.analysis_dir.mkdir(parents=True, exist_ok=True)
        self.configs_dir.mkdir(parents=True, exist_ok=True)
        self.generated_tests_dir.mkdir(parents=True, exist_ok=True)

        # Run steps
        success = True

        if not self.step_analyze_code():
            self.log("ERROR: Code analysis failed")
            success = False

        if success and not self.step_extract_configs():
            self.log("WARNING: Config extraction had issues")
            # Don't fail, continue

        if success and not self.step_generate_tests():
            self.log("WARNING: Test generation had issues")
            # Don't fail, continue

        self.step_create_report()

        if success:
            self.log("\n" + "="*60)
            self.log("WORKFLOW COMPLETED SUCCESSFULLY!")
            self.log("="*60)

            if self.dry_run:
                self.log("\nThis was a DRY RUN.")
                self.log("Review the results in .ai-tools/ directory")
                self.log("Run with --apply to make actual changes")
        else:
            self.log("\nWORKFLOW COMPLETED WITH ERRORS")

        return success


def main():
    parser = argparse.ArgumentParser(
        description='AI-Powered Code Improvement Workflow',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Dry run (analyze only, no changes)
  python ai_workflow.py

  # Apply changes
  python ai_workflow.py --apply

  # Run specific steps
  python ai_workflow.py --step analyze
  python ai_workflow.py --step extract --apply
  python ai_workflow.py --step tests
        '''
    )

    parser.add_argument('--apply', action='store_true',
                       help='Apply changes (default is dry run)')
    parser.add_argument('--quiet', action='store_true',
                       help='Minimal output')
    parser.add_argument('--step', choices=['analyze', 'extract', 'tests', 'all'],
                       default='all',
                       help='Run specific step (default: all)')

    args = parser.parse_args()

    workflow = AIWorkflow(
        dry_run=not args.apply,
        verbose=not args.quiet
    )

    if args.step == 'all':
        success = workflow.run_full_workflow()
    elif args.step == 'analyze':
        success = workflow.step_analyze_code()
    elif args.step == 'extract':
        success = workflow.step_extract_configs()
    elif args.step == 'tests':
        success = workflow.step_generate_tests()

    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()

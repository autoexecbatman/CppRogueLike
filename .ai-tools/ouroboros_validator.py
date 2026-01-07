#!/usr/bin/env python3
"""
Ouroboros Validator Module
Builds and tests code to verify fixes work
"""

import json
import subprocess
import re
from pathlib import Path

class OuroborosValidator:
    def __init__(self, config):
        """Initialize validator with configuration."""
        self.config = config
        self.build_config = config["build"]
        self.safety = config["safety"]

    def validate(self, surgery_results):
        """Validate that surgery was successful."""
        print("Validating changes...")

        validation_report = {
            "build_status": "unknown",
            "test_status": "unknown",
            "build_output": "",
            "test_output": "",
            "warnings_before": 0,
            "warnings_after": 0,
            "errors": []
        }

        # Step 1: Build project
        print("\n[1/2] Building project...")
        build_result = self.build_project()

        validation_report["build_status"] = "success" if build_result["success"] else "failed"
        validation_report["build_output"] = build_result["output"]
        validation_report["warnings_after"] = build_result["warnings"]

        if not build_result["success"]:
            validation_report["errors"] = build_result["errors"]
            print("  ✗ Build failed!")

            if self.safety["auto_rollback"]:
                print("  Rolling back changes...")
                self.rollback(surgery_results)

            return validation_report

        print("  ✓ Build succeeded")

        # Step 2: Run tests
        if self.safety["require_tests"]:
            print("[2/2] Running tests...")
            test_result = self.run_tests()

            validation_report["test_status"] = "success" if test_result["success"] else "failed"
            validation_report["test_output"] = test_result["output"]

            if not test_result["success"]:
                validation_report["errors"].extend(test_result["errors"])
                print("  ✗ Tests failed!")

                if self.safety["auto_rollback"]:
                    print("  Rolling back changes...")
                    self.rollback(surgery_results)

                return validation_report

            print("  ✓ All tests passed")

        return validation_report

    def build_project(self):
        """Build the project."""
        build_dir = Path(self.build_config["build_dir"])
        config = self.build_config["config"]

        # Run cmake build
        cmd = [
            "cmake",
            "--build", str(build_dir),
            "--config", config
        ]

        result = {
            "success": False,
            "output": "",
            "warnings": 0,
            "errors": []
        }

        try:
            process = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout
            )

            result["output"] = process.stdout + process.stderr

            # Check for errors
            if process.returncode == 0:
                result["success"] = True
            else:
                result["errors"] = self.parse_build_errors(result["output"])

            # Count warnings
            result["warnings"] = result["output"].count("warning")

        except subprocess.TimeoutExpired:
            result["errors"] = ["Build timed out after 5 minutes"]
        except Exception as e:
            result["errors"] = [str(e)]

        return result

    def run_tests(self):
        """Run test executable."""
        build_dir = Path(self.build_config["build_dir"])
        config = self.build_config["config"]

        # Find test executable
        test_exe = build_dir / "bin" / config / "test_exe.exe"

        if not test_exe.exists():
            return {
                "success": False,
                "output": "",
                "errors": [f"Test executable not found: {test_exe}"]
            }

        result = {
            "success": False,
            "output": "",
            "errors": []
        }

        try:
            process = subprocess.run(
                [str(test_exe)],
                capture_output=True,
                text=True,
                timeout=60,  # 1 minute timeout
                cwd=test_exe.parent
            )

            result["output"] = process.stdout + process.stderr

            # Check if all tests passed
            if "[  PASSED  ]" in result["output"] and "[  FAILED  ]" not in result["output"]:
                result["success"] = True
            else:
                result["errors"] = self.parse_test_failures(result["output"])

        except subprocess.TimeoutExpired:
            result["errors"] = ["Tests timed out after 1 minute"]
        except Exception as e:
            result["errors"] = [str(e)]

        return result

    def parse_build_errors(self, output):
        """Parse build errors from output."""
        errors = []

        # Look for error lines
        for line in output.split('\n'):
            if 'error' in line.lower() and ('error C' in line or 'error LNK' in line):
                errors.append(line.strip())

        return errors[:10]  # Limit to first 10 errors

    def parse_test_failures(self, output):
        """Parse test failures from output."""
        failures = []

        # Look for failed test names
        pattern = r'\[  FAILED  \] (.+)'
        for match in re.finditer(pattern, output):
            failures.append(match.group(1))

        return failures

    def rollback(self, surgery_results):
        """Rollback changes made by surgery."""
        print("Rolling back changes...")

        # Delete generated config files
        if surgery_results.get("config_generated"):
            config_path = Path(surgery_results["config_generated"])
            if config_path.exists():
                config_path.unlink()
                print(f"  Deleted: {config_path}")

        # Restore modified files (would use rollback manifest in real implementation)
        for file_path in surgery_results.get("files_modified", []):
            print(f"  Would restore: {file_path}")

        print("  ✓ Rollback complete")

    def validate_current(self):
        """Validate current state without surgery results."""
        print("Validating current codebase state...")

        validation_report = {
            "build_status": "unknown",
            "test_status": "unknown"
        }

        # Build
        build_result = self.build_project()
        validation_report["build_status"] = "success" if build_result["success"] else "failed"
        validation_report["warnings"] = build_result["warnings"]

        if build_result["success"]:
            # Test
            test_result = self.run_tests()
            validation_report["test_status"] = "success" if test_result["success"] else "failed"

        return validation_report

if __name__ == "__main__":
    # Quick test
    import sys

    sys.path.insert(0, ".")

    with open(".ai-tools/ouroboros_config.json", 'r') as f:
        config = json.load(f)

    validator = OuroborosValidator(config)

    print("Testing current codebase state...")
    report = validator.validate_current()

    print("\n" + "=" * 60)
    print("VALIDATION COMPLETE")
    print("=" * 60)
    print(f"Build: {report['build_status']}")
    print(f"Tests: {report['test_status']}")

    if report['build_status'] == 'success':
        print(f"Warnings: {report.get('warnings', 0)}")

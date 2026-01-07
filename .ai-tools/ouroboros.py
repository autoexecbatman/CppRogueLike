#!/usr/bin/env python3
"""
Ouroboros: Self-Healing Code Maintenance System
Main orchestrator for automated code refactoring and maintenance.
"""

import argparse
import json
import sys
import os
from pathlib import Path
from datetime import datetime

class Ouroboros:
    def __init__(self, config_path=".ai-tools/ouroboros_config.json"):
        """Initialize Ouroboros with configuration."""
        self.config = self.load_config(config_path)
        self.output_dir = Path(self.config["paths"]["output_dir"])
        self.output_dir.mkdir(parents=True, exist_ok=True)

        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.branch_name = f"ouroboros-refactor-{self.timestamp}"

    def load_config(self, config_path):
        """Load Ouroboros configuration."""
        with open(config_path, 'r') as f:
            return json.load(f)

    def run_scan(self):
        """Phase 1: Scan codebase for issues."""
        print("=" * 60)
        print("OUROBOROS - Phase 1: Scanning Codebase")
        print("=" * 60)

        # Import scanner module
        from ouroboros_scanner import OuroborosScanner

        scanner = OuroborosScanner(self.config)
        health_report = scanner.scan()

        # Save health report
        report_path = self.output_dir / "health_report.json"
        with open(report_path, 'w') as f:
            json.dump(health_report, f, indent=2)

        print(f"\n✓ Health report saved: {report_path}")
        print(f"\nSummary:")
        print(f"  Magic numbers:    {health_report['summary']['magic_numbers']}")
        print(f"  String literals:  {health_report['summary']['string_literals']}")
        print(f"  Long functions:   {health_report['summary']['long_functions']}")
        print(f"  Duplicates:       {health_report['summary']['duplicates']}")

        return health_report

    def run_triage(self, health_report):
        """Phase 2: Prioritize and batch issues."""
        print("\n" + "=" * 60)
        print("OUROBOROS - Phase 2: Triaging Issues")
        print("=" * 60)

        from ouroboros_triage import OuroborosTriage

        triage = OuroborosTriage(self.config)
        triage_queue = triage.prioritize(health_report)

        # Save triage queue
        queue_path = self.output_dir / "triage_queue.json"
        with open(queue_path, 'w') as f:
            json.dump(triage_queue, f, indent=2)

        print(f"\n✓ Triage queue saved: {queue_path}")
        print(f"\nBatches created: {len(triage_queue['batches'])}")

        return triage_queue

    def run_surgery(self, triage_queue, dry_run=True):
        """Phase 3: Generate and apply fixes."""
        print("\n" + "=" * 60)
        print("OUROBOROS - Phase 3: Generating Fixes")
        print("=" * 60)

        if dry_run:
            print("⚠️  DRY RUN MODE - No files will be modified")

        from ouroboros_surgeon import OuroborosSurgeon

        surgeon = OuroborosSurgeon(self.config)
        surgery_results = surgeon.perform_surgery(triage_queue, dry_run=dry_run)

        # Save surgery results
        results_path = self.output_dir / "surgery_results.json"
        with open(results_path, 'w') as f:
            json.dump(surgery_results, f, indent=2)

        print(f"\n✓ Surgery results saved: {results_path}")

        return surgery_results

    def run_validation(self, surgery_results):
        """Phase 4: Verify fixes compile and pass tests."""
        print("\n" + "=" * 60)
        print("OUROBOROS - Phase 4: Validating Changes")
        print("=" * 60)

        from ouroboros_validator import OuroborosValidator

        validator = OuroborosValidator(self.config)
        validation_report = validator.validate(surgery_results)

        # Save validation report
        report_path = self.output_dir / "validation_report.json"
        with open(report_path, 'w') as f:
            json.dump(validation_report, f, indent=2)

        print(f"\n✓ Validation report saved: {report_path}")

        if validation_report["build_status"] == "success":
            print("\n✓ BUILD: PASSED")
        else:
            print("\n✗ BUILD: FAILED")

        if validation_report["test_status"] == "success":
            print("✓ TESTS: PASSED")
        else:
            print("✗ TESTS: FAILED")

        return validation_report

    def run_presentation(self, health_report, surgery_results, validation_report):
        """Phase 5: Generate human-readable report."""
        print("\n" + "=" * 60)
        print("OUROBOROS - Phase 5: Generating Report")
        print("=" * 60)

        from ouroboros_presenter import OuroborosPresenter

        presenter = OuroborosPresenter(self.config)
        report_markdown = presenter.create_report(
            health_report,
            surgery_results,
            validation_report,
            self.branch_name
        )

        # Save markdown report
        report_path = self.output_dir / "OUROBOROS_REPORT.md"
        with open(report_path, 'w') as f:
            f.write(report_markdown)

        print(f"\n✓ Report saved: {report_path}")
        print("\n" + "=" * 60)
        print("OUROBOROS COMPLETE")
        print("=" * 60)
        print(f"\nNext steps:")
        print(f"  1. Review: cat {report_path}")
        print(f"  2. Test:   git checkout {self.branch_name}")
        print(f"  3. Merge:  git merge {self.branch_name}")
        print(f"  4. Commit: git commit -m 'Refactor: Ouroboros automated fixes'")

        return report_path

    def run_full(self, dry_run=True):
        """Run complete Ouroboros pipeline."""
        print("\n" + "=" * 60)
        print("OUROBOROS - Self-Healing Code Maintenance System")
        print("=" * 60)
        print(f"Timestamp: {self.timestamp}")
        print(f"Mode: {'DRY RUN' if dry_run else 'APPLY CHANGES'}")
        print("=" * 60)

        try:
            # Phase 1: Scan
            health_report = self.run_scan()

            # Phase 2: Triage
            triage_queue = self.run_triage(health_report)

            # Phase 3: Surgery
            surgery_results = self.run_surgery(triage_queue, dry_run=dry_run)

            if not dry_run:
                # Phase 4: Validation (only in apply mode)
                validation_report = self.run_validation(surgery_results)

                # Check if validation passed
                if validation_report["build_status"] != "success" or \
                   validation_report["test_status"] != "success":
                    print("\n⚠️  Validation failed! Rolling back changes...")
                    # Rollback will be handled by validator
                    return False
            else:
                # In dry run, create mock validation report
                validation_report = {
                    "build_status": "skipped",
                    "test_status": "skipped",
                    "warnings_before": 0,
                    "warnings_after": 0
                }

            # Phase 5: Presentation
            self.run_presentation(health_report, surgery_results, validation_report)

            return True

        except Exception as e:
            print(f"\n✗ Error: {e}")
            import traceback
            traceback.print_exc()
            return False

def main():
    parser = argparse.ArgumentParser(
        description="Ouroboros: Self-Healing Code Maintenance System"
    )

    parser.add_argument(
        '--mode',
        choices=['scan', 'full', 'verify', 'report'],
        default='full',
        help='Execution mode (default: full)'
    )

    parser.add_argument(
        '--apply',
        action='store_true',
        help='Apply changes (default is dry-run)'
    )

    parser.add_argument(
        '--batch-size',
        type=int,
        default=10,
        help='Number of issues per batch (default: 10)'
    )

    parser.add_argument(
        '--type',
        choices=['magic_numbers', 'strings', 'functions', 'duplicates', 'all'],
        default='all',
        help='Issue type to fix (default: all)'
    )

    args = parser.parse_args()

    # Create Ouroboros instance
    ouroboros = Ouroboros()

    # Override batch size if specified
    if args.batch_size:
        ouroboros.config['limits']['batch_size'] = args.batch_size

    # Run based on mode
    if args.mode == 'scan':
        ouroboros.run_scan()
    elif args.mode == 'full':
        dry_run = not args.apply
        success = ouroboros.run_full(dry_run=dry_run)
        sys.exit(0 if success else 1)
    elif args.mode == 'verify':
        # Just run validation on current state
        from ouroboros_validator import OuroborosValidator
        validator = OuroborosValidator(ouroboros.config)
        validation_report = validator.validate_current()
        print(json.dumps(validation_report, indent=2))
    elif args.mode == 'report':
        # Show last report
        report_path = Path(ouroboros.output_dir) / "OUROBOROS_REPORT.md"
        if report_path.exists():
            with open(report_path, 'r') as f:
                print(f.read())
        else:
            print("No report found. Run with --mode full first.")

if __name__ == "__main__":
    main()

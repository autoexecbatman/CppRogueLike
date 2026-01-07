#!/usr/bin/env python3
"""
Ouroboros Presenter Module
Generates human-readable reports
"""

import json
from datetime import datetime
from pathlib import Path

class OuroborosPresenter:
    def __init__(self, config):
        """Initialize presenter with configuration."""
        self.config = config

    def create_report(self, health_report, surgery_results, validation_report, branch_name):
        """Create markdown report."""
        lines = []

        # Header
        lines.append("# Ouroboros Fix Report")
        lines.append(f"**Date**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        lines.append(f"**Branch**: `{branch_name}`")
        lines.append("")

        # Summary
        lines.append("## Summary")
        lines.append("")

        batches_processed = len(surgery_results.get("batches_processed", []))
        files_modified = len(surgery_results.get("files_modified", []))

        lines.append(f"- Batches processed: {batches_processed}")
        lines.append(f"- Files modified: {files_modified}")
        lines.append(f"- Build status: **{validation_report['build_status'].upper()}**")
        lines.append(f"- Test status: **{validation_report['test_status'].upper()}**")
        lines.append("")

        # Health scan results
        lines.append("## Codebase Health Scan")
        lines.append("")
        lines.append("| Issue Type | Count |")
        lines.append("|------------|-------|")
        for issue_type, count in health_report["summary"].items():
            lines.append(f"| {issue_type.replace('_', ' ').title()} | {count} |")
        lines.append("")

        # Changes applied
        if batches_processed > 0:
            lines.append("## Changes Applied")
            lines.append("")

            for batch_info in surgery_results["batches_processed"]:
                lines.append(f"### Batch: {batch_info['batch_id']}")
                lines.append(f"**Type**: {batch_info['type']}")
                lines.append(f"**Issues Fixed**: {batch_info['issues_fixed']}")
                lines.append("")

                # Show config structure if generated
                if "result" in batch_info and "config_structure" in batch_info["result"]:
                    lines.append("**Generated Configuration**:")
                    lines.append("```json")
                    lines.append(json.dumps(batch_info["result"]["config_structure"], indent=2))
                    lines.append("```")
                    lines.append("")

        # Files modified
        if files_modified > 0:
            lines.append("## Modified Files")
            lines.append("")
            for file_path in surgery_results.get("files_modified", []):
                lines.append(f"- `{file_path}`")
            lines.append("")

        # Validation results
        lines.append("## Verification Results")
        lines.append("")

        if validation_report["build_status"] == "success":
            lines.append("### Build: PASSED")
            warnings = validation_report.get("warnings_after", 0)
            lines.append(f"- Warnings: {warnings}")
        else:
            lines.append("### Build: FAILED")
            lines.append("")
            lines.append("**Errors**:")
            lines.append("```")
            for error in validation_report.get("errors", []):
                lines.append(error)
            lines.append("```")

        lines.append("")

        if validation_report["test_status"] == "success":
            lines.append("### Tests: PASSED")
            lines.append("- All tests passing")
        elif validation_report["test_status"] == "failed":
            lines.append("### Tests: FAILED")
            lines.append("")
            lines.append("**Failed Tests**:")
            for error in validation_report.get("errors", []):
                lines.append(f"- {error}")
        else:
            lines.append("### Tests: SKIPPED (Dry Run)")

        lines.append("")

        # Next steps
        lines.append("## Next Steps")
        lines.append("")

        if validation_report["build_status"] == "success" and validation_report["test_status"] == "success":
            lines.append("All changes validated successfully!")
            lines.append("")
            lines.append("1. **Review changes**:")
            lines.append(f"   ```bash")
            lines.append(f"   git diff master..{branch_name}")
            lines.append(f"   ```")
            lines.append("")
            lines.append("2. **Test manually** (optional):")
            lines.append(f"   ```bash")
            lines.append(f"   git checkout {branch_name}")
            lines.append(f"   cmake --build build && build/bin/Debug/C++RogueLike.exe")
            lines.append(f"   ```")
            lines.append("")
            lines.append("3. **Merge changes**:")
            lines.append(f"   ```bash")
            lines.append(f"   git checkout master")
            lines.append(f"   git merge {branch_name}")
            lines.append(f"   ```")
            lines.append("")
            lines.append("4. **Commit**:")
            lines.append(f"   ```bash")
            lines.append(f"   git commit -m 'Refactor: Ouroboros automated config extraction'")
            lines.append(f"   ```")
        else:
            lines.append("Validation failed. Changes have been rolled back.")
            lines.append("")
            lines.append("Review the errors above and fix manually, or run Ouroboros again.")

        lines.append("")
        lines.append("---")
        lines.append("*Generated by Ouroboros: Self-Healing Code Maintenance System*")

        return "\n".join(lines)

if __name__ == "__main__":
    # Quick test
    import sys

    sys.path.insert(0, ".")

    with open(".ai-tools/ouroboros_config.json", 'r') as f:
        config = json.load(f)

    # Load all reports
    health_path = Path(".ai-tools/output/health_report.json")
    surgery_path = Path(".ai-tools/output/surgery_results.json")
    validation_path = Path(".ai-tools/output/validation_report.json")

    if not all(p.exists() for p in [health_path, surgery_path]):
        print("Missing report files. Run full Ouroboros pipeline first.")
        sys.exit(1)

    with open(health_path, 'r') as f:
        health_report = json.load(f)

    with open(surgery_path, 'r') as f:
        surgery_results = json.load(f)

    # Mock validation if doesn't exist
    if validation_path.exists():
        with open(validation_path, 'r') as f:
            validation_report = json.load(f)
    else:
        validation_report = {
            "build_status": "skipped",
            "test_status": "skipped",
            "warnings_after": 0
        }

    presenter = OuroborosPresenter(config)
    report = presenter.create_report(
        health_report,
        surgery_results,
        validation_report,
        "ouroboros-test-branch"
    )

    print(report)

    # Save report
    output_path = Path(".ai-tools/output/OUROBOROS_REPORT.md")
    with open(output_path, 'w') as f:
        f.write(report)

    print(f"\nReport saved: {output_path}")

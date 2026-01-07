#!/usr/bin/env python3
"""
Ouroboros Triage Module
Prioritizes and batches issues for fixing
"""

import json
from collections import defaultdict

class OuroborosTriage:
    def __init__(self, config):
        """Initialize triage with configuration."""
        self.config = config
        self.priorities = config["priorities"]
        self.batch_size = config["limits"]["batch_size"]

    def prioritize(self, health_report):
        """Prioritize issues and create fix batches."""
        print("Prioritizing issues...")

        all_issues = []

        # Collect all issues with priority scores
        for issue_type, issues in health_report["issues"].items():
            priority = self.priorities.get(issue_type, 50)

            for issue in issues:
                all_issues.append({
                    "type": issue_type,
                    "priority": priority,
                    "severity": issue.get("severity", "medium"),
                    "issue": issue
                })

        # Sort by priority (higher first), then severity
        severity_order = {"high": 3, "medium": 2, "low": 1}
        all_issues.sort(
            key=lambda x: (
                -x["priority"],
                -severity_order.get(x["severity"], 2)
            )
        )

        # Create batches
        batches = []
        current_batch = []
        current_type = None

        for item in all_issues:
            issue_type = item["type"]

            # Start new batch if type changes or batch is full
            if current_type != issue_type or len(current_batch) >= self.batch_size:
                if current_batch:
                    batches.append(self.create_batch(current_type, current_batch))
                current_batch = []
                current_type = issue_type

            current_batch.append(item["issue"])

        # Add remaining batch
        if current_batch:
            batches.append(self.create_batch(current_type, current_batch))

        # Create triage queue
        triage_queue = {
            "timestamp": health_report["timestamp"],
            "total_issues": len(all_issues),
            "batches": batches
        }

        print(f"Created {len(batches)} batches from {len(all_issues)} issues")

        return triage_queue

    def create_batch(self, issue_type, issues):
        """Create a batch from issues."""
        batch = {
            "batch_id": f"BATCH_{len(issues):03d}_{issue_type.upper()}",
            "type": issue_type,
            "priority": self.priorities.get(issue_type, 50),
            "issue_count": len(issues),
            "issues": issues,
            "estimated_time": self.estimate_time(issue_type, len(issues))
        }

        return batch

    def estimate_time(self, issue_type, count):
        """Estimate time to fix issues."""
        # Time estimates in minutes
        time_per_issue = {
            "magic_numbers": 0.5,  # 30 seconds per magic number
            "string_literals": 0.3,  # 20 seconds per string
            "long_functions": 2,  # 2 minutes per function
            "duplicates": 3,  # 3 minutes per duplicate
            "build_errors": 5,  # 5 minutes per error
            "test_failures": 3  # 3 minutes per test
        }

        base_time = time_per_issue.get(issue_type, 1) * count
        # Add overhead for AI processing
        ai_overhead = 1
        # Add overhead for validation
        validation_overhead = 3

        total = base_time + ai_overhead + validation_overhead

        if total < 1:
            return f"{int(total * 60)} seconds"
        else:
            return f"{int(total)} minutes"

if __name__ == "__main__":
    # Quick test
    import sys
    from pathlib import Path

    sys.path.insert(0, ".")

    with open(".ai-tools/ouroboros_config.json", 'r') as f:
        config = json.load(f)

    # Load health report
    report_path = Path(".ai-tools/output/health_report.json")
    if not report_path.exists():
        print("No health report found. Run scanner first.")
        sys.exit(1)

    with open(report_path, 'r') as f:
        health_report = json.load(f)

    triage = OuroborosTriage(config)
    queue = triage.prioritize(health_report)

    print("\n" + "=" * 60)
    print("TRIAGE COMPLETE")
    print("=" * 60)
    for i, batch in enumerate(queue["batches"], 1):
        print(f"Batch {i}: {batch['type']} ({batch['issue_count']} issues, {batch['estimated_time']})")

    # Save queue
    output_path = Path(".ai-tools/output/triage_queue.json")
    with open(output_path, 'w') as f:
        json.dump(queue, f, indent=2)

    print(f"\nQueue saved: {output_path}")

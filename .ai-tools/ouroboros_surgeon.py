#!/usr/bin/env python3
"""
Ouroboros Surgeon Module
Uses AI to generate JSON configs and refactor code
"""

import json
import re
import subprocess
from pathlib import Path
from collections import defaultdict

class OuroborosSurgeon:
    def __init__(self, config):
        """Initialize surgeon with configuration."""
        self.config = config
        self.ai_config = config["ai"]
        self.refactoring = config["refactoring"]
        self.output_dir = Path(config["paths"]["output_dir"])

    def perform_surgery(self, triage_queue, dry_run=True):
        """Perform AI-powered surgery on code."""
        print(f"Performing surgery (dry_run={dry_run})...")

        results = {
            "batches_processed": [],
            "config_generated": None,
            "files_modified": [],
            "rollback_info": []
        }

        # Process first batch only (for now)
        if not triage_queue["batches"]:
            print("No batches to process")
            return results

        batch = triage_queue["batches"][0]
        print(f"\nProcessing batch: {batch['batch_id']} ({batch['type']})")

        if batch["type"] == "magic_numbers":
            # Generate JSON config and refactor code
            surgery_result = self.refactor_magic_numbers(batch["issues"], dry_run)
            results["batches_processed"].append({
                "batch_id": batch["batch_id"],
                "type": batch["type"],
                "issues_fixed": len(batch["issues"]),
                "result": surgery_result
            })
            results["config_generated"] = surgery_result.get("config_path")
            results["files_modified"] = surgery_result.get("files_modified", [])

        return results

    def refactor_magic_numbers(self, magic_numbers, dry_run=True):
        """Refactor magic numbers to JSON config."""
        print(f"Refactoring {len(magic_numbers)} magic numbers...")

        # Step 1: Group magic numbers by semantic meaning using AI
        print("\n[1/5] Analyzing and grouping magic numbers...")
        grouped_config = self.ai_group_magic_numbers(magic_numbers)

        # Step 2: Generate hierarchical JSON config
        print("[2/5] Generating game_config.json...")
        config_json = self.generate_json_config(grouped_config)

        # Step 3: Generate Config.h with type-safe structs
        print("[3/5] Generating Config.h...")
        config_header = self.generate_config_header(grouped_config)

        # Step 4: Generate Config.cpp with initialization
        print("[4/5] Generating Config.cpp...")
        config_impl = self.generate_config_impl(grouped_config)

        # Step 5: Refactor source files to use Config
        print("[5/5] Refactoring source files...")
        refactored_files = self.refactor_source_files(magic_numbers, grouped_config, dry_run)

        if not dry_run:
            # Write files
            self.write_config_files(config_json, config_header, config_impl)

        return {
            "config_path": self.refactoring["config_json"],
            "header_path": self.refactoring["config_header"],
            "impl_path": self.refactoring["config_impl"],
            "files_modified": refactored_files,
            "config_structure": grouped_config
        }

    def ai_group_magic_numbers(self, magic_numbers):
        """Use AI to group magic numbers semantically."""
        print("  Querying AI for semantic grouping...")

        # Prepare context for AI
        magic_context = []
        for magic in magic_numbers[:20]:  # Limit to first 20 for performance
            magic_context.append({
                "value": magic["value"],
                "file": magic["file"],
                "line": magic["line"],
                "context": magic["context"][:200]  # Truncate context
            })

        # Build AI prompt
        prompt = f"""Analyze these magic numbers from C++ code and organize them into a hierarchical JSON structure.

Magic numbers found:
{json.dumps(magic_context, indent=2)}

Requirements:
1. Group by logical category (items, combat, ai, map, ui, etc.)
2. Create subcategories as needed (potions, weapons, detection, generation, etc.)
3. Use snake_case for all keys
4. Suggest descriptive names based on usage context
5. Return ONLY valid JSON with this structure:

{{
  "category": {{
    "subcategory": {{
      "descriptive_name": {{
        "value": <number>,
        "type": "int|float",
        "cpp_name": "member_name"
      }}
    }}
  }}
}}

Example:
{{"items": {{"potions": {{"health_potion_heal": {{"value": 10, "type": "int", "cpp_name": "health_potion_heal"}}}}}}}}

Return ONLY the JSON, no explanation."""

        # Query AI
        try:
            response = self.query_ollama(prompt)
            # Parse JSON from response
            config_structure = self.extract_json_from_response(response)
            print(f"  ✓ AI grouped magic numbers into {len(config_structure)} categories")
            return config_structure
        except Exception as e:
            print(f"  ✗ AI grouping failed: {e}")
            # Fallback to simple grouping by file
            return self.fallback_grouping(magic_numbers)

    def query_ollama(self, prompt, temperature=0.3):
        """Query Ollama AI model."""
        import requests

        url = f"{self.ai_config['base_url']}/api/generate"
        payload = {
            "model": self.ai_config["model"],
            "prompt": prompt,
            "stream": False,
            "options": {
                "temperature": temperature
            }
        }

        response = requests.post(url, json=payload, timeout=self.ai_config["timeout"])
        response.raise_for_status()

        return response.json()["response"]

    def extract_json_from_response(self, response):
        """Extract JSON from AI response."""
        # Try to find JSON in response
        # Look for outermost {}
        start = response.find('{')
        end = response.rfind('}')

        if start != -1 and end != -1:
            json_str = response[start:end+1]
            return json.loads(json_str)
        else:
            raise ValueError("No JSON found in response")

    def fallback_grouping(self, magic_numbers):
        """Fallback grouping when AI fails."""
        print("  Using fallback grouping...")

        grouped = {
            "general": {}
        }

        for i, magic in enumerate(magic_numbers[:10]):  # Limit to 10
            key = f"value_{i}"
            grouped["general"][key] = {
                "value": magic["value"],
                "type": "int" if isinstance(magic["value"], int) else "float",
                "cpp_name": key
            }

        return grouped

    def generate_json_config(self, grouped_config):
        """Generate hierarchical JSON config file content."""
        # Extract just values for JSON
        json_config = {}

        for category, subcategories in grouped_config.items():
            json_config[category] = {}
            for subcat, values in subcategories.items():
                json_config[category][subcat] = {}
                for name, info in values.items():
                    json_config[category][subcat][name] = info["value"]

        return json.dumps(json_config, indent=2)

    def generate_config_header(self, grouped_config):
        """Generate Config.h with type-safe structs."""
        lines = []

        lines.append("#pragma once")
        lines.append("#include <nlohmann/json.hpp>")
        lines.append("#include <fstream>")
        lines.append("#include <stdexcept>")
        lines.append("")

        # Generate struct for each category
        for category, subcategories in grouped_config.items():
            struct_name = f"{category.capitalize()}Config"
            lines.append(f"struct {struct_name} {{")

            # Flatten subcategories into members
            for subcat, values in subcategories.items():
                for name, info in values.items():
                    cpp_type = info["type"]
                    cpp_name = info["cpp_name"]
                    lines.append(f"    {cpp_type} {cpp_name};")

            lines.append("};")
            lines.append("")

        # Generate main Config class
        lines.append("class Config {")
        lines.append("private:")
        lines.append("    static bool initialized;")
        lines.append("    static nlohmann::json config_data;")
        lines.append("")
        lines.append("    static void load() {")
        lines.append("        std::ifstream file(\"config/game_config.json\");")
        lines.append("        if (!file.is_open()) {")
        lines.append("            throw std::runtime_error(\"Failed to load config/game_config.json\");")
        lines.append("        }")
        lines.append("        file >> config_data;")
        lines.append("    }")
        lines.append("")
        lines.append("public:")

        # Static members for each category
        for category in grouped_config.keys():
            struct_name = f"{category.capitalize()}Config"
            lines.append(f"    static {struct_name} {category};")

        lines.append("")
        lines.append("    static void init() {")
        lines.append("        if (initialized) return;")
        lines.append("        load();")

        # Load each category
        for category, subcategories in grouped_config.items():
            for subcat, values in subcategories.items():
                for name, info in values.items():
                    cpp_name = info["cpp_name"]
                    json_path = f'["{category}"]["{subcat}"]["{name}"]'
                    lines.append(f"        {category}.{cpp_name} = config_data{json_path};")

        lines.append("        initialized = true;")
        lines.append("    }")
        lines.append("};")
        lines.append("")

        return "\n".join(lines)

    def generate_config_impl(self, grouped_config):
        """Generate Config.cpp with static member initialization."""
        lines = []

        lines.append("#include \"Config/Config.h\"")
        lines.append("")
        lines.append("bool Config::initialized = false;")
        lines.append("nlohmann::json Config::config_data;")
        lines.append("")

        # Initialize static members
        for category in grouped_config.keys():
            struct_name = f"{category.capitalize()}Config"
            lines.append(f"{struct_name} Config::{category}{{}};")

        lines.append("")

        return "\n".join(lines)

    def refactor_source_files(self, magic_numbers, grouped_config, dry_run=True):
        """Refactor source files to use Config."""
        files_modified = []

        # Group magic numbers by file
        by_file = defaultdict(list)
        for magic in magic_numbers:
            by_file[magic["file"]].append(magic)

        # Process each file
        for file_path, magics in by_file.items():
            print(f"  Refactoring: {file_path}")

            if not dry_run:
                # Read file
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()

                # Add include if needed
                if "#include \"Config/Config.h\"" not in content:
                    # Find first include
                    lines = content.split('\n')
                    for i, line in enumerate(lines):
                        if '#include' in line:
                            lines.insert(i+1, '#include "Config/Config.h"')
                            break
                    content = '\n'.join(lines)

                # Replace magic numbers (simplified for now)
                # In real implementation, would use AI to generate replacements

                # Write back
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)

            files_modified.append(file_path)

        return files_modified

    def write_config_files(self, config_json, config_header, config_impl):
        """Write config files to disk."""
        # Create directories
        config_dir = Path(self.refactoring["config_json"]).parent
        config_dir.mkdir(parents=True, exist_ok=True)

        header_dir = Path(self.refactoring["config_header"]).parent
        header_dir.mkdir(parents=True, exist_ok=True)

        # Write JSON config
        with open(self.refactoring["config_json"], 'w') as f:
            f.write(config_json)
        print(f"  ✓ Wrote {self.refactoring['config_json']}")

        # Write header
        with open(self.refactoring["config_header"], 'w') as f:
            f.write(config_header)
        print(f"  ✓ Wrote {self.refactoring['config_header']}")

        # Write implementation
        with open(self.refactoring["config_impl"], 'w') as f:
            f.write(config_impl)
        print(f"  ✓ Wrote {self.refactoring['config_impl']}")

if __name__ == "__main__":
    # Quick test
    import sys

    sys.path.insert(0, ".")

    with open(".ai-tools/ouroboros_config.json", 'r') as f:
        config = json.load(f)

    # Load triage queue
    queue_path = Path(".ai-tools/output/triage_queue.json")
    if not queue_path.exists():
        print("No triage queue found. Run triage first.")
        sys.exit(1)

    with open(queue_path, 'r') as f:
        triage_queue = json.load(f)

    surgeon = OuroborosSurgeon(config)
    results = surgeon.perform_surgery(triage_queue, dry_run=True)

    print("\n" + "=" * 60)
    print("SURGERY COMPLETE (DRY RUN)")
    print("=" * 60)
    print(f"Batches processed: {len(results['batches_processed'])}")
    print(f"Files would be modified: {len(results['files_modified'])}")

    # Save results
    output_path = Path(".ai-tools/output/surgery_results.json")
    with open(output_path, 'w') as f:
        json.dump(results, f, indent=2)

    print(f"\nResults saved: {output_path}")

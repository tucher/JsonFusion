#!/usr/bin/env python3
"""
Script to automatically update README.md with benchmark results
Reads JSON output from build.py and updates the tables in README
"""

import json
import re
import sys
from pathlib import Path
from typing import Dict, List


class ReadmeUpdater:
    def __init__(self, readme_path: Path, results_dir: Path):
        self.readme_path = readme_path
        self.results_dir = results_dir
        self.readme_content = ""
        
    def load_readme(self):
        """Load README.md content"""
        with open(self.readme_path, 'r') as f:
            self.readme_content = f.read()
    
    def save_readme(self):
        """Save updated README.md"""
        with open(self.readme_path, 'w') as f:
            f.write(self.readme_content)
    
    def load_results(self, platform_name: str) -> Dict:
        """Load benchmark results JSON for a platform"""
        # Normalize platform name to match filename
        filename = f"results_{platform_name.lower().replace(' ', '_').replace('(', '').replace(')', '')}.json"
        results_file = self.results_dir / filename
        
        if not results_file.exists():
            print(f"Warning: Results file not found: {results_file}")
            return None
        
        with open(results_file, 'r') as f:
            return json.load(f)
    
    def format_arm_table(self, results: Dict) -> str:
        """Format ARM Cortex-M table"""
        if not results or 'results' not in results:
            return ""
        
        # Extract M7 and M0+ results
        m7_results = {}
        m0p_results = {}
        
        for config_name, libs in results['results'].items():
            if 'M7' in config_name:
                m7_results = libs
            elif 'M0+' in config_name:
                m0p_results = libs
        
        if not m7_results or not m0p_results:
            return ""
        
        # Get versions
        versions = results.get('versions', {})
        
        # Build table rows with sorting
        rows = []
        for lib_name in m7_results.keys():
            m7_size = m7_results.get(lib_name, {}).get('kb', 0)
            m0p_size = m0p_results.get(lib_name, {}).get('kb', 0)
            version = versions.get(lib_name, "")
            
            rows.append((lib_name, m7_size, m0p_size, version))
        
        # Sort by M7 size
        rows.sort(key=lambda x: x[1])
        
        # Format rows
        formatted_rows = []
        for lib_name, m7_size, m0p_size, version in rows:
            # Format library name
            display_name = lib_name
            if lib_name == "JsonFusion" or "JsonFusion" in lib_name:
                display_name = f"**{lib_name}**"
            
            formatted_rows.append(f"| {display_name:37} | {m7_size:5.1f} KB | {m0p_size:5.1f} KB | {version:10} |")
        
        # Build complete table
        table = "| Library                               |     M7    |    M0+    |  Version   |\n"
        table += "|---------------------------------------|-----------|-----------|------------|\n"
        table += "\n".join(formatted_rows)
        
        return table
    
    def format_esp32_table(self, results: Dict) -> str:
        """Format ESP32 table"""
        if not results or 'results' not in results:
            return ""
        
        # Extract ESP32 results (should be only one config)
        esp32_results = {}
        for config_name, libs in results['results'].items():
            esp32_results = libs
            break
        
        if not esp32_results:
            return ""
        
        # Get versions
        versions = results.get('versions', {})
        
        # Build table rows with sorting
        rows = []
        for lib_name in esp32_results.keys():
            size = esp32_results.get(lib_name, {}).get('kb', 0)
            version = versions.get(lib_name, "")
            
            rows.append((lib_name, size, version))
        
        # Sort by size
        rows.sort(key=lambda x: x[1])
        
        # Format rows
        formatted_rows = []
        for lib_name, size, version in rows:
            # Format library name
            display_name = lib_name
            if lib_name == "JsonFusion" or "JsonFusion" in lib_name:
                display_name = f"**{lib_name}**"
            
            formatted_rows.append(f"| {display_name:37} | {size:5.1f} KB | {version:10} |")
        
        # Build complete table
        table = "| Library                               |   ESP32   |  Version   |\n"
        table += "|---------------------------------------|-----------|------------|\n"
        table += "\n".join(formatted_rows)
        
        return table
    
    def update_arm_table(self, results: Dict):
        """Update ARM Cortex-M table in README"""
        new_table = self.format_arm_table(results)
        if not new_table:
            print("Error: Could not format ARM table")
            return False
        
        # Find and replace ARM table
        # Pattern: matches the table between the header and the next "**Key Takeaways:**"
        # Updated to handle optional Version column
        pattern = r'(\|\s*Library\s*\|.*?\n\|[-\s|]+\|.*?\n)((?:\|[^\n]+\|\n)+)(?=\n\*\*Key Takeaways:\*\*)'
        
        def replacer(match):
            # Replace entire table (header + data)
            return new_table + '\n'
        
        updated_content = re.sub(pattern, replacer, self.readme_content, flags=re.DOTALL)
        
        if updated_content == self.readme_content:
            print("Warning: ARM table pattern not found or no changes made")
            return False
        
        self.readme_content = updated_content
        return True
    
    def update_esp32_table(self, results: Dict):
        """Update ESP32 table in README"""
        new_table = self.format_esp32_table(results)
        if not new_table:
            print("Error: Could not format ESP32 table")
            return False
        
        # Find and replace ESP32 table
        # Pattern: matches the ESP32 table
        # Look for table that comes after "#### ESP32" and before "**Key Takeaways:**"
        pattern = r'(\|\s*Library\s*\|.*?\n\|[-\s|]+\|.*?\n)((?:\|[^\n]+\|\n)+)(?=\n\*\*Key Takeaways:\*\*)'
        
        # Find the ESP32 section first
        esp32_section_start = self.readme_content.find('#### ESP32')
        if esp32_section_start == -1:
            print("Warning: Could not find ESP32 section")
            return False
        
        # Apply replacement only after ESP32 section
        before_esp32 = self.readme_content[:esp32_section_start]
        after_esp32 = self.readme_content[esp32_section_start:]
        
        def replacer(match):
            # Replace entire table (header + data)
            return new_table + '\n'
        
        updated_after = re.sub(pattern, replacer, after_esp32, count=1, flags=re.DOTALL)
        
        if updated_after == after_esp32:
            print("Warning: ESP32 table pattern not found or no changes made")
            return False
        
        self.readme_content = before_esp32 + updated_after
        return True
    
    def run(self):
        """Main update process"""
        print("=" * 60)
        print("README.md Table Updater")
        print("=" * 60)
        print()
        
        # Load README
        print(f"Loading README: {self.readme_path}")
        self.load_readme()
        
        # Update ARM table
        print("\nUpdating ARM Cortex-M table...")
        arm_results = self.load_results("arm_cortex-m")
        if arm_results:
            if self.update_arm_table(arm_results):
                print("✓ ARM table updated")
                # Print versions
                if 'versions' in arm_results:
                    print("  Versions:")
                    for lib, ver in arm_results['versions'].items():
                        print(f"    {lib}: {ver}")
            else:
                print("✗ ARM table update failed")
        
        # Update ESP32 table
        print("\nUpdating ESP32 table...")
        esp32_results = self.load_results("esp32_xtensa_lx6")
        if esp32_results:
            if self.update_esp32_table(esp32_results):
                print("✓ ESP32 table updated")
                # Print versions
                if 'versions' in esp32_results:
                    print("  Versions:")
                    for lib, ver in esp32_results['versions'].items():
                        print(f"    {lib}: {ver}")
            else:
                print("✗ ESP32 table update failed")
        
        # Save README
        print("\nSaving README...")
        self.save_readme()
        print("✓ Done!")
        print()


def main():
    # Determine paths
    script_dir = Path(__file__).parent.resolve()
    readme_path = script_dir.parent.parent.parent / "README.md"
    results_dir = script_dir
    
    if not readme_path.exists():
        print(f"Error: README.md not found at {readme_path}")
        sys.exit(1)
    
    # Run updater
    updater = ReadmeUpdater(readme_path, results_dir)
    updater.run()


if __name__ == "__main__":
    main()

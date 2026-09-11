#!/usr/bin/env python3
"""
Master test runner for PAL/Ø Sieve Framework

Runs all sieves in sequence. Fail any, fail all.
"""

import sys
import subprocess
from pathlib import Path

SIEVES = [
    "lexical_sieve",
    "syntactic_sieve",
    "semantic_sieve",
    "behavioral_sieve"
]

def run_sieve(name):
    print(f"\n{'='*60}")
    print(f"RUNNING {name.upper()} SIEVE")
    print('='*60)
    
    try:
        result = subprocess.run([sys.executable, f"{name}.py"], cwd="tests/sieve", capture_output=True, text=True)
        if result.returncode == 0:
            print(result.stdout)
            return True
        else:
            print("❌ FAILED:")
            print(result.stderr)
            return False
    except Exception as e:
        print(f"💥 Exception running {name}: {e}")
        return False

if __name__ == "__main__":
    all_passed = True
    
    for sieve in SIEVES:
        if not run_sieve(sieve):
            all_passed = False
    
    print("\n" + "="*60)
    if all_passed:
        print("🎉 ALL SIEVES PASSED — PAL/Ø compiler infrastructure is validated!")
    else:
        print("💥 ONE OR MORE SIEVES FAILED — Compiler not ready for agent use.")
        sys.exit(1)
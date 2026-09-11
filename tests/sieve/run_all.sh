#!/bin/bash

# Run all sieve tests in order
cd "$(dirname "$0")"

echo "🧪 Starting PAL/Ø Sieve Test Suite..."

python3 test_all.py

if [ $? -eq 0 ]; then
    echo "✅ SUCCESS: All sieves passed."
else
    echo "❌ FAILURE: One or more sieves failed."
    exit 1
fi
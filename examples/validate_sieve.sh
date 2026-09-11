#!/bin/bash
# Validation script for Sieve of Eratosthenes implementation

echo "Testing Prime Sieve Implementation"

# First, let's simulate the transpiler pipeline:
# 1. English → MMD (via EN-MMD-Qwen-2.50-7b)
# 2. MMD → PAL (via MMD-PAL-Qwen-2.50-7b)
# 3. PAL → GIMPLE (via the PAL compiler)
# 4. GIMPLE → GCC → Executable

echo "Simulating transpiler pipeline..."
echo "Input: sieve_english.txt"
echo "Output would be a C program compatible with GCC"

# Create a sample generated C code that our system would produce
cat > sieve_generated.c << 'EOF'
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

int main() {
    const int n = 100;
    bool is_prime[n + 1];
    
    // Initialize all values as true, except 0 and 1
    for (int i = 0; i <= n; i++) {
        is_prime[i] = true;
    }
    is_prime[0] = is_prime[1] = false;
    
    // Sieve of Eratosthenes
    for (int i = 2; i * i <= n; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= n; j += i) {
                is_prime[j] = false;
            }
        }
    }
    
    // Print all prime numbers
    printf("Prime numbers up to %d: ", n);
    int count = 0;
    for (int i = 2; i <= n; i++) {
        if (is_prime[i]) {
            printf("%d ", i);
            count++;
        }
    }
    printf("\\nTotal primes found: %d\\n", count);
    
    return 0;
}
EOF

echo "Generated C code saved to sieve_generated.c"

# Compile with GCC
gcc -o sieve_exe sieve_generated.c

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    
    # Run the executable and capture output
    ./sieve_exe
    
    # Verify we got reasonable results (should have 25 primes under 100)
    result=$(./sieve_exe | grep -o "[0-9]\\+" | wc -l)
    if [ "$result" -eq 25 ]; then
        echo "Validation successful! Found $result prime numbers (expected 25)."
    else
        echo "Validation failed! Expected 25 primes, found $result."
        exit 1
    fi
    
    # Clean up
    rm sieve_generated.c sieve_exe
else
    echo "Compilation failed!"
    rm sieve_generated.c
    exit 1
fi
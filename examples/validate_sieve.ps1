# PowerShell validation script for Sieve of Eratosthenes implementation

Write-Host "Testing Prime Sieve Implementation"

# Create a sample generated C code that our system would produce
$code = @'
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
'@

# Save the C code to file
$code | Out-File -FilePath "sieve_generated.c" -Encoding ASCII
Write-Host "Generated C code saved to sieve_generated.c"

# Check if gcc is available
if (!(Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Host "GCC not found. Please install GCC (e.g., via MinGW or WSL) to compile the program."
    exit 1
}

# Compile with GCC
Write-Host "Compiling with GCC..."
gcc -o sieve_exe sieve_generated.c

if ($LASTEXITCODE -eq 0) {
    Write-Host "Compilation successful!"
    
    # Run the executable and capture output
    $output = .\sieve_exe
    
    Write-Host $output
    
    # Verify we got reasonable results (should have 25 primes under 100)
    $primes = ($output -split '\s+' | Where-Object { $_ -match '^\d+$' } | Measure-Object).Count
    if ($primes -eq 25) {
        Write-Host "Validation successful! Found $primes prime numbers (expected 25)."
    } else {
        Write-Host "Validation failed! Expected 25 primes, found $primes."
        exit 1
    }
    
    # Clean up
    Remove-Item sieve_generated.c
    Remove-Item sieve_exe.exe -ErrorAction SilentlyContinue
} else {
    Write-Host "Compilation failed!"
    Remove-Item sieve_generated.c -ErrorAction SilentlyContinue
    exit 1
}
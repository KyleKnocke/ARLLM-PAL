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

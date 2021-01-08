//
// Created by matob on 2. 1. 2021.
//

#ifndef CHATAPP_RSA_H
#define CHATAPP_RSA_H
int first1000Primes[1000];

void initFirst1000Primes() {
    int lastPrime = 1;
    for (int i = 0; i < 1000; i++) {
        int current = lastPrime;
        bool isPrime = false;
        while (!isPrime) {
            isPrime = true;
            current++;
            for (int j = 0; j < i; j++) {
                if (current % first1000Primes[j] == 0) {
                    isPrime = false;
                    break;
                }
            }
        }
        first1000Primes[i] = current;
        lastPrime = current;
    }
}

unsigned long long int gcd(unsigned long long int int1, unsigned long long int int2) {
    while (int1 != int2) {
        if (int1 > int2)
            int1 = int1 % int2;
        if (int1 == 0) {
            int1 += int2;
        } else
            int2 = int2 % int1;
        if (int2 == 0) {
            int2 += int1;
        }
    }
    return int1;
}

unsigned long long int powLint(unsigned long long int prvy, unsigned long long int druhy) {
    unsigned long long int result = 1;
    for (int i = 0; i < druhy; i++) {
        result *= prvy;
    }
    return result;
}

unsigned long long int
modularPow(unsigned long long int prvy, unsigned long long int druhy, unsigned long long int mod) {
    unsigned long long int result = 1;
    prvy = prvy % mod;
    if (prvy == 0)
        return 0;

    while (druhy > 0) {
        if (druhy & 1)
            result = (result * prvy) % mod;
        druhy = druhy >> 1;
        prvy = (prvy * prvy) % mod;
    }
    return result;
}

bool divideFirstPrimesTest(unsigned long long int possiblePrime) {
    for (int i = 0; i < 1000; i++) {
        if (possiblePrime % first1000Primes[i] == 0) {
            return false;
        }
    }
    return true;
}

bool
millerRabinTestIteration(unsigned long long int divisionByTwo, unsigned long long int tester, unsigned long long int d,
                         unsigned long long int possiblePrime) {
    //if(powLint(tester,d)%possiblePrime == 1) {
    unsigned long long int modPow = modularPow(tester, d, possiblePrime);
    if (modPow == 1) {
        return true;
    }
    for (int i = 0; i < divisionByTwo; i++) {
        unsigned long long int doc = modularPow(tester, powLint(2, i) * d, possiblePrime);
        //unsigned long long int doc = (powLint(tester,powLint(2,i)*d)%possiblePrime);
        //unsigned long long int doc = (pow(tester,pow(2,i)*d,possiblePrime));
        if (doc == (possiblePrime - 1)) {
            return true;
        }
    }
    return false;
}

bool millerRabinTest(unsigned long long int possiblePrime) {
    //20 iterations
    unsigned long long int divisionByTwo = 0;
    unsigned long long int d = possiblePrime - 1;
    while (d % 2 == 0) {
        d /= 2;
        divisionByTwo++;
    }


    for (int i = 0; i < 20; i++) {
        unsigned long long int round_tester = 2 + rand() % (possiblePrime - 2);
        if (!millerRabinTestIteration(divisionByTwo, round_tester, d, possiblePrime)) {
            return false;
        }

    }
    return true;
}

unsigned long long int genPrime() {
    unsigned long long int nBit = 16;
    unsigned long long int prime =
            (powLint(2, nBit - 1) + 1) + rand() % ((powLint(2, nBit) - powLint(2, nBit - 1) - 1));
    bool isPrime = false;
    while (!isPrime) {
        prime = (powLint(2, nBit - 1) + 1) + rand() % ((powLint(2, nBit) - powLint(2, nBit - 1) - 1));
        while (!divideFirstPrimesTest(prime)) {
            prime = (powLint(2, nBit - 1) + 1) + rand() % ((powLint(2, nBit) - powLint(2, nBit - 1) - 1));
        }
        if (millerRabinTest(prime)) {
            isPrime = true;
        }
    }
    return prime;
}

unsigned long long int *generujKluceRSA() {

    unsigned long long int prime1 = genPrime();
    unsigned long long int prime2 = genPrime();
    unsigned long long int n = prime1 * prime2;
    unsigned long long int totient = (prime1 - 1) * (prime2 - 1);

    unsigned long long int e = 1;
    for (unsigned long long int i = 2; i < totient; i++) {
        if (gcd(i, totient) == 1) {
            e = i;
            break;
        }
    }

    unsigned long long int d = 1;
    unsigned long long int k = 1;
    bool findedD = false;
    while (!findedD) {
        unsigned long long int x = 1 + k * totient;
        if (x % e == 0) {
            d = x / e;
            findedD = true;
        }
        k++;
    }

    unsigned long long int *keys = malloc(3 * sizeof(unsigned long long int));
    keys[0] = e; // public key
    keys[1] = n; // public key
    keys[2] = d; // private key
    return keys;
}

unsigned long long int *zasifruj(char *mes, unsigned long long int e, unsigned long long int n) {
    unsigned long long int *cipher = malloc(256 * sizeof(unsigned long long int));
    bzero(cipher, 256 * sizeof(unsigned long long int));
    for (int i = 0; i < 255; i++) {
        cipher[i] = modularPow((unsigned long long int) mes[i], e, n);
    }
    return cipher;
}

char *desifruj(unsigned long long int *mes, unsigned long long int d, unsigned long long int n) {
    char *decipher = malloc(256 * sizeof(char));
    bzero(decipher, 256);
    for (int i = 0; i < 255; i++) {
        decipher[i] = (char) modularPow((unsigned long long int) mes[i], d, n);
    }
    return decipher;
}

#endif //CHATAPP_RSA_H

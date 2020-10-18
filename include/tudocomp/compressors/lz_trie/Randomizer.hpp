#pragma once

#include <cstdint>
#include <iostream>

namespace tdc { namespace lz_trie {

class Randomizer {
public:
    Randomizer(uint64_t universeSize);

    ~Randomizer();

    inline uint64_t hash(uint64_t key) const {
        return ((key % _prime) * _a) % _prime;
    }

    inline uint64_t invertHash(uint64_t hash) const {
        return (hash * _aInv) % _prime;
    }

    const uint64_t _prime;
private:
    // hashfunction
    uint64_t getModInverse(uint64_t a, uint64_t prime);

    uint64_t nextPrimeNumber(uint64_t inputNumber);

    bool isPrime(uint64_t input);

    void euclAlgorithm(uint64_t prime);

    uint64_t _universeSize;
    uint64_t _a;     // some magic number, depends on prime
    uint64_t _aInv;  // a^(-1). used to get original key from quotient
};


Randomizer::Randomizer(uint64_t universeSize)
        : _prime(nextPrimeNumber(universeSize - 1)) {
    _universeSize = universeSize;
//    _prime = nextPrimeNumber(universeSize - 1);
    euclAlgorithm(_prime);   // would calculate some random number 'a' & its inverse 'aInv'
}

Randomizer::~Randomizer() {

}



/*
 * Function for determining the next prime number
 */
uint64_t Randomizer::nextPrimeNumber(uint64_t inputNumber) {
    uint64_t nextPrimeNumber;
    if (inputNumber <= 0) {
        std::cout << "The number you have entered is zero or negative.\n";
    } else {
        while (inputNumber != 0) {  //TODO: WHY????
            nextPrimeNumber = inputNumber + 1;
            if (nextPrimeNumber % 2 == 0 && nextPrimeNumber != 2) {
                nextPrimeNumber += 1;
            }
            while (!isPrime(nextPrimeNumber)) {
                nextPrimeNumber += 2;
            }
            if (isPrime(nextPrimeNumber))
                return nextPrimeNumber;
        }
    }
    return nextPrimeNumber;
} // end nextPrimeNumber

/* Function that checks whether or not a given number is
 * a prime number or not.
 */
bool Randomizer::isPrime(uint64_t input) {
    uint64_t i;
    bool prime = true;

    if (input == 2) {
        return true;
    }

    if (input % 2 == 0 || input <= 1) {
        prime = false;
    } else {
        for (i = 3; i <= sqrt(input); i += 2) {
            if (input % i == 0) {
                prime = false;
            }
        }
    }
    return prime;
} // end isPrime

// A naive method to find modulor multiplicative inverse of
// 'a' under modulo 'm'
uint64_t Randomizer::getModInverse(uint64_t a, uint64_t prime) {
    a = a % prime;
    for (uint64_t x = 1; x < prime; x++)
        if ((a * x) % prime == 1)
            return x;
    return prime;
}

void Randomizer::euclAlgorithm(uint64_t prime) {
    long long aTemp = (long long) (0.66 * (double) prime);
    long long aInvTemp = prime;
    while (true) {
        aInvTemp = getModInverse(aTemp, prime);
        if (aInvTemp == prime)
            aTemp++;
        else
            break;
    }
    _a = aTemp;  // TODO: aTemp is 'long long int', meaning signed. a is 'unsigned int64'
    _aInv = aInvTemp; // TODO: the same
}

}}//ns

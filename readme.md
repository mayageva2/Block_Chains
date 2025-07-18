# Multi-Threaded Password Encryption & Brute-Force Decryption

## Authors

- Maya Geva
- Victoria Musiyko
- Gal Rubinstein

---

## Overview

This project implements a multi-threaded C program that demonstrates password encryption and brute-force decryption using a "MTA Crypto" library. The system includes:

- An encrypter thread that generates a random printable password, encrypts it using a randomly generated key, and shares it with all decrypter threads.
- Multiple decrypter threads that attempt to brute-force the key used for encryption.
- POSIX threads (`pthreads`) for synchronization.
- Shared memory structure and condition variables to manage communication between threads.

---

## Setup Instructions

1. **Clone the Repository**  
   ```bash
   git clone https://github.com/mayageva2/Block_Chains.git
   cd Block_Chains
   ```

2. **Build the Project**  
   Use the provided `Makefile`:
   ```bash
   make
   ```

3. **Run the Program**  
   Run the program from the terminal:
   ```bash
   ./build/encrypt.out -n <num-decrypters> -l <password-length> [-t <timeout-seconds>]
   ```
   
   - -n | --num-of-decrypters :	(Required) Amount of decrypter threads which will be created
   - -l | --password-length :		(Required) Number of characters that will be encrypted
   - -t | --timeout :				(Optional) Time in seconds until server regenerates a password if it didn't receive correctly decrypted password
   
   For example,
   ```bash
   ./build/encrypt.out -n 4 -l 32 -t 10
   ```
   
   This will:
   - Create 4 decrypter threads
   - Use a 32-character password
   - Set a timeout of 10 seconds before generating a new password if not decrypted

---

## Program Features

- **Multithreaded Decryption:**  
  Spawns multiple decrypter threads to efficiently brute-force encrypted passwords in parallel.

- **Thread Synchronization:**  
  Uses mutexes and condition variables to coordinate between the encrypter and decrypter threads safely.

- **Dynamic Password Generation:**  
  Continuously generates random printable passwords of configurable length.

- **Graceful Timeout Handling**  
  Supports optional timeout to automatically regenerate password to decrypt.


---

## License

This repository is intended for educational purposes only.

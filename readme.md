# Multi-Threaded Password Encryption & Brute-Force Decryption

## Authors

- Maya Geva
- Victoria Musiyko
- Gal Rubinstein

---

## Overview

This project implements a multi-process Dockerized C program that demonstrates password encryption and brute-force decryption using a "MTA Crypto" library. The system includes:

- Encryptor container that generates a random printable password, encrypts it using a randomly generated key, and distributes it via named pipe.
- Multiple Decryptor containers, each attempting to brute-force the key using the encrypted password.
- Named Pipes (FIFOs) in a shared volume (/mnt/mta) for inter-process communication.
- Log files per container under /var/log/, which can be inspected after the program finishes.

---

## Setup Instructions

1. **Clone the Repository**  
   ```bash
   git clone https://github.com/mayageva2/Block_Chains.git
   cd Block_Chains
   ```

2. **Build the Docker Images and Run the Program**  
   Use the command:
   make NUM_DECRYPTERS=<X> TIMEOUT=<T> PASSWORD_LENGTH=<L>
   
   For example,
   make NUM_DECRYPTERS=3 TIMEOUT=10 PASSWORD_LENGTH=16
   
   This will:
   - Start an encrypter container
   - Start 3 decrypter containers
   - Will set the password length to 16
   - Set a timeout of 10 seconds before generating a new password if not decrypted
   
3. **Stop the Program**  
    Press CTRL+C in the terminal where launcher.sh is running.
This will terminate both the running processes and their containers immediately.
    
4. **Cleanup**  
    Use the command: make clean
    This will: 
   - Stop and remove all containers
   - Delete Docker images
   - Remove the shared volume ./mnt/mta/
              
5. **Print Log Files**  
    You can view each container's log file using command: cat mnt/logs/*file_name*.log
    
    For example,
    cat mnt/logs/decrypter_1.log
 
---

## Program Features

- **Containerized Brute-Force Decryption:**  
  Runs each component (encrypter and multiple decrypters) in separate Docker containers for isolated   execution and easy management
    
- **Inter-Process Communication with Named Pipes:**  
  Utilizes named pipes (FIFO) in a shared volume (/mnt/mta) to transfer encrypted passwords and guesses between the encrypter and decrypters.

- **Dynamic Password Generation:**  
  Continuously generates random printable passwords of configurable length.

- **Graceful Timeout Handling:**  
  Supports optional timeout to automatically regenerate password to decrypt.
  
  - **Logging Per Container:**  
  Each container writes detailed logs to a dedicated file in /var/log, allowing real-time monitoring from separate terminals.
  
---

## License

This repository is intended for educational purposes only.

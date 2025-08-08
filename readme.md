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
   
2. **Pull Docker Images**  
   Run commands:
   - docker pull victoriamus/encrypter_img
   - docker pull victoriamus/decrypter_img

3. **Run the Program**  
   Use the command:
   ./launcher.sh <num-decrypters> <password-length> <timeout-seconds> 
   (you can also choose not to add timeout feature, then run ./launcher.sh <num-decrypters> <password-length>)
   
   For example,
  ./launcher.sh 3 16 10
  
   This will:
   - Set the password length to 16
   - Set a timeout of 10 seconds before generating a new password if not decrypted
   - run the program
   
4. **Stop the Program**  
    Open a new terminal window, navigate to the same directory where you ran the program, and run the following command: ./stop.sh 

This will stop the containers immediately.
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

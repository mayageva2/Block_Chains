#!/bin/bash

# Stop and remove the encrypter container 
docker rm -f encrypter 2>/dev/null || true

# Stop and remove all decrypter containers 
docker ps -aqf "name=decrypter_" | xargs -r docker rm -f

echo "All encrypter and decrypter containers have been stopped and removed."


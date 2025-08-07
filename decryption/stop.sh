#!/bin/bash

# Stop the encrypter container 
docker stop encrypter 2>/dev/null || true

# Stop all decrypter containers 
docker ps -aqf "name=decrypter_" | xargs -r docker stop

echo "All encrypter and decrypter containers have been stopped."


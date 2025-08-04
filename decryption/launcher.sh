#!/bin/bash
# Get the number of decrypters, timeout duration, and password length
NUM_DECRYPTERS=$1
TIMEOUT=$2
PASSWORD_LENGTH=$3

# Set up shared volume path for named pipes and config file
VOLUME_PATH="./mnt/mta"
mkdir -p "$VOLUME_PATH"

# Create configuration file with the password length
echo "password_length=$PASSWORD_LENGTH" > "$VOLUME_PATH/mtacrypt.conf"

# Start encrypter container
docker run --rm \
  --name encrypter \
  -v "$VOLUME_PATH":/mnt/mta \
  encrypter_img \
  ./encrypter -t "$TIMEOUT" &
ENCRYPTER_PID=$!
  
# Give encrypter time to create the pipe
sleep 1

# Start decrypters container
for i in $(seq 1 "$NUM_DECRYPTERS"); do
  docker run --rm \
    --name "decrypter_$i" \
    -e DECRYPTER_ID=$i \
    -v "$VOLUME_PATH":/mnt/mta \
    decrypter_img &
done

#Define what to do when Ctrl+C is pressed 
trap 'echo "[LAUNCHER] Stopping decrypters..."; \
      docker ps -q --filter "name=decrypter_" | xargs -r docker stop >/dev/null 2>&1; \
      echo "[LAUNCHER] Stopping encrypter..."; \
      docker stop encrypter >/dev/null 2>&1; \
      exit 0' SIGINT
      
# Wait for the encrypter container to finish
wait $ENCRYPTER_PID


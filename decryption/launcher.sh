#!/bin/bash
# Get the number of decrypters, timeout duration, and password length
NUM_DECRYPTERS=$1
PASSWORD_LENGTH=$2
TIMEOUT=$3

#check number of decrypters
if [[ "$NUM_DECRYPTERS" -lt 1 ]]; then
  echo "ERROR: Number of decrypters must be at least 1."
  exit 1
fi

#check positive number for TIMEOUT
if [[ -n "$TIMEOUT" && "$TIMEOUT" -lt 0 ]]; then
  echo "ERROR: Timeout must be 0 or greater."
  exit 1
fi

# Remove existing containers if they exist
docker rm -f encrypter 2>/dev/null || true
docker ps -aqf "name=decrypter_" | xargs -r docker rm -f

# Set up shared volume path for named pipes and config file and logs file
VOLUME_PATH="./mnt/mta"
mkdir -p "$VOLUME_PATH" 

# Create configuration file with the password length
echo "password_length=$PASSWORD_LENGTH" > "$VOLUME_PATH/mtacrypt.conf"

if [[ -n "$TIMEOUT" ]]; then
 docker run \
  --name encrypter \
  -v "$VOLUME_PATH":/mnt/mta \
  victoriamus/encrypter_img \
  ./encrypter -t "$TIMEOUT" &
else
 docker run \
 --name encrypter \
  -v "$VOLUME_PATH":/mnt/mta \
  victoriamus/encrypter_img \
  ./encrypter &
fi

sleep 1

for i in $(seq 1 "$NUM_DECRYPTERS"); do
  docker run \
    --name "decrypter_$i" \
    -e DECRYPTER_ID=$i \
    -v "$VOLUME_PATH":/mnt/mta \
    victoriamus/decrypter_img &

done
     


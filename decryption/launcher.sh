#!/bin/bash
# Get the number of decrypters, timeout duration, and password length
NUM_DECRYPTERS=$1
TIMEOUT=$2
PASSWORD_LENGTH=$3

# Remove existing containers if they exist
docker rm -f encrypter 2>/dev/null || true
docker ps -aqf "name=decrypter_" | xargs -r docker rm -f

#Build encrypter and decrypter images
docker build -f Dockerfile.encrypter -t encrypter_img .
docker build -f Dockerfile.decrypter -t decrypter_img .

# Set up shared volume path for named pipes and config file and logs file
VOLUME_PATH="./mnt/mta"
LOG_VOLUME="./mnt/logs"
mkdir -p "$VOLUME_PATH" "$LOG_VOLUME"

# Create configuration file with the password length
echo "password_length=$PASSWORD_LENGTH" > "$VOLUME_PATH/mtacrypt.conf"


# Start encrypter container
sudo docker run --rm \
  --name encrypter \
  -v "$VOLUME_PATH":/mnt/mta \
  -v "$LOG_VOLUME":/var/log  \
  encrypter_img \
  ./encrypter -t "$TIMEOUT" &

sleep 1

for i in $(seq 1 "$NUM_DECRYPTERS"); do
  sudo docker run --rm \
    --name "decrypter_$i" \
    -e DECRYPTER_ID=$i \
    -v "$VOLUME_PATH":/mnt/mta \
    -v "$LOG_VOLUME":/var/log  \
    decrypter_img &

done

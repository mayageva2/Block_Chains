# Default target: build Docker images and run launcher script
all: docker-build run

# Build Docker images for encrypter and decrypters
docker-build:
	docker build -f Dockerfile.encrypter -t encrypter_img .
	docker build -f Dockerfile.decrypter -t decrypter_img .
	
#run containrs through script launch.sh	and clean afterwards
run:
	chmod +x launcher.sh
	./launcher.sh $(NUM_DECRYPTERS) $(TIMEOUT) $(PASSWORD_LENGTH)

.PHONY: clean
clean:
	@echo "[MAKE] Cleaning up containers, images, and volume directory..."
	-@docker rm -f encrypter 2>/dev/null || true   
	-@docker ps -aqf "name=decrypter_" | xargs -r docker rm -f >/dev/null 2>&1 || true
	-@docker rmi -f encrypter_img decrypter_img >/dev/null 2>&1 || true
	-@rm -rf ./mnt

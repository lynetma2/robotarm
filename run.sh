sudo podman run -it --privileged --device=/dev/ttyAMA0:/dev/ttyAMA0 --device=/dev/hidraw0:/dev/hidraw0 localhost/robotarm:latest /bin/bash

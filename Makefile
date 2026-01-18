upload:
	pio run -t upload

upload_external_esptool: build external_esptool

build:
	pio run

external_esptool:
	esptool --chip esp32 --port "socket://localhost:3232" --baud 115200 --before default-reset --after hard-reset write-flash -z --flash-mode dio --flash-freq 40m --flash-size 4MB 0x1000 .pio/build/dev/bootloader.bin 0x8000 .pio/build/dev/partitions.bin 0x10000 .pio/build/dev/firmware.bin

monitor:
	pio device monitor

refresh:
	pio init --ide vim
	python3 conv.py

check:
	pio check

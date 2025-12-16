upload:
	pio run -t upload

monitor:
	pio device monitor

refresh:
	pio init --ide vim
	python3 conv.py

check:
	pio check

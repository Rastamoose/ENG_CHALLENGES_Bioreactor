# fully qualified board name - package:architecture:board:board_options(optional)
FQBN := arduino:avr:uno
ifneq ($(wildcard /usr/share/arduino/hardware/archlinux-arduino/avr/boards.txt),)
export ARDUINO_DIRECTORIES_USER := /usr/share/arduino
FQBN := archlinux-arduino:avr:uno
endif

# serial monitor baud rate
BAUD := 115200
PORT := /dev/ttyACM0

BUILD := BUILD

.PHONY: all upload monitor db clean

all:
	arduino-cli compile --fqbn $(FQBN) --build-path $(BUILD)

upload: all
	arduino-cli upload --fqbn $(FQBN) -p $(PORT) --input-dir $(BUILD)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

db:
	arduino-cli compile --fqbn $(FQBN) --build-path $(BUILD) --only-compilation-database

clean:
	rm -rf $(BUILD)

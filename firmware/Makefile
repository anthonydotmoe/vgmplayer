run:
	./pico.sh reset
	inotifywait -e CLOSE_NOWRITE --format "%f" --quiet /dev --monitor | while read i; do if [ "$$i" = ttyACM0 ]; then break; fi; done
	picocom -b115200 /dev/ttyACM0

CC=gcc
CFLAGS=-Wall -I.
DEBUG=-ggdb -fsanitize=address -fsanitize=undefined
LIBS=-lueye_api

ueye_driver:
	$(CC) -o ueye_driver main.c driver.c process.c $(LIBS)

debug:
	$(CC) $(DEBUG) -o ueye_driver main.c driver.c process.c $(LIBS)

clean:
	rm ./ueye_driver

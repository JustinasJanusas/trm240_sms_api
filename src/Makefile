BIN?=trm240_sms_api
LIBS:=

.PHONY: all clean rebuild

all: $(BIN)

$(BIN): main.c serial_util.c string_util.c
	gcc $^ -o $@ $(LIBS) -lzmq -ljson-c



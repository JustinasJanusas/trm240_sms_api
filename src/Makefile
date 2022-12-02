BIN:=trm240_sms_api
LIBS:=

.PHONY: all clean rebuild

all: $(BIN)

rebuild: clean $(BIN)

$(BIN): main.c serial_util.c
	gcc $^ -o $@ $(LIBS)

clean:
	rm -f $(BIN)


BIN:=../build/trm240_sms_api
export
.PHONY: all clean rebuild

all: $(BIN)

rebuild: clean $(BIN)

$(BIN): 
	mkdir -p build
	make -C src

clean:
	rm -f $(BIN)
	rmdir build


.PHONY: all
all: bin/lokios.0

bin/lokios.0:
	cd lokios.0 && $(MAKE)
	cp lokios.0/lokios.0 bin/lokios.0

.PHONY: clean
clean:
	cd lokios.0 && $(MAKE) clean
	rm -f bin/*

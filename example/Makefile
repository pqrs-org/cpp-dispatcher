all:
	mkdir -p build \
		&& cd build \
		&& cmake .. \
		&& make
	make run

clean:
	rm -r build

run:
	./build/example

update_vendor:
	cget install -f cget-requirements.txt --prefix vendor

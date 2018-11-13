all:
	mkdir -p build \
		&& cd build \
		&& cmake .. \
		&& make
	make run

clean:
	rm -rf build

run:
	./build/example

update_vendor:
	rm -rf vendor
	cget install pqrs-org/cpp-cget-recipes --prefix vendor
	cget install -f cget-requirements.txt --prefix vendor

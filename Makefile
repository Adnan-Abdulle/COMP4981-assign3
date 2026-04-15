.PHONY: build test

build:
	./build.sh

test: build
	python3 tests/integration_test.py ./build/main

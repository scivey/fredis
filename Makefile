deps:
	./scripts/deps.sh

clean:
	rm -rf build

create-runner: deps
	mkdir -p build && cd build && cmake ../ && make runner -j8

run: create-runner
	./build/runner

.PHONY: run create-runner

deps:
	./scripts/deps.sh

create-runner: deps
	mkdir -p build && cd build && cmake ../ && make runner -j8

run: create-runner
	./build/runner

.PHONY: run create-runner

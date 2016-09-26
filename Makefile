deps:
	./scripts/deps.sh

clean:
	rm -rf build

base: deps
	mkdir -p build && cd build && cmake ../

create-runner: base
	cd build && make runner -j8

run: create-runner
	./build/runner

create-integration: base
	cd build && make integration_runner -j8

integration: create-integration
	./build/integration_runner

.PHONY: run create-runner

.PHONY: all build test test-race coverage coverage-html fmt fmt-check vet clean

all: build

build:
	go build ./...

test:
	go test -v ./...

test-race:
	go test -v -race ./...

coverage:
	mkdir -p build
	go test -coverprofile=build/coverage.out ./...
	go tool cover -func=build/coverage.out

coverage-html:
	mkdir -p build
	go test -coverprofile=build/coverage.out ./...
	go tool cover -html=build/coverage.out -o build/coverage.html

fmt:
	gofmt -w .

fmt-check:
	@test -z "$$(gofmt -l .)" || (echo "Code is not formatted. Run 'make fmt'" && gofmt -l . && exit 1)

vet:
	go vet ./...

clean:
	rm -rf build/

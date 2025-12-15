FROM golang:1.22-alpine

RUN apk add --no-cache make bash

# Install go-test-report for HTML test reports
RUN go install github.com/vakenbolt/go-test-report@latest

WORKDIR /app/implementations/go

COPY implementations/go/ ./
COPY test-vectors/ /app/test-vectors/

CMD ["sh", "-c", "make clean && make fmt-check && make vet && make build && make test && make coverage && make docs"]

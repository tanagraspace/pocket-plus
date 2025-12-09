FROM golang:1.22-alpine

RUN apk add --no-cache make bash

WORKDIR /app/implementations/go

COPY implementations/go/ ./
COPY test-vectors/ /app/test-vectors/

CMD ["sh", "-c", "make clean && make build && make test && make coverage && make docs"]

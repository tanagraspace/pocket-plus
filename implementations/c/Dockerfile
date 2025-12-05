FROM alpine:edge

RUN apk add --no-cache gcc musl-dev make lcov bc gzip --repository=http://dl-cdn.alpinelinux.org/alpine/edge/testing

WORKDIR /app/implementations/c

COPY implementations/c/ ./
COPY test-vectors/ /app/test-vectors/

CMD ["sh", "-c", "make clean && make && make coverage && make coverage-html"]

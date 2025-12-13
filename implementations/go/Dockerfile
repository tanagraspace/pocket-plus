FROM golang:1.22-alpine

RUN apk add --no-cache make bash python3 py3-pip

# Install gotestsum for JUnit XML output and junit2html for HTML conversion
RUN go install gotest.tools/gotestsum@latest && \
    pip install --break-system-packages junit2html

WORKDIR /app/implementations/go

COPY implementations/go/ ./
COPY test-vectors/ /app/test-vectors/

CMD ["sh", "-c", "make clean && make build && make test && make coverage && make docs"]

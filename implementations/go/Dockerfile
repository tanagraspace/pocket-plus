FROM golang:1.22-alpine

RUN apk add --no-cache make bash python3 py3-pip

# Install gotestsum for JUnit XML output and junit2html for HTML conversion
# Pin gotestsum to v1.12.0 for Go 1.22 compatibility (v1.13+ requires Go 1.24+)
RUN go install gotest.tools/gotestsum@v1.12.0 && \
    pip install --break-system-packages junit2html

WORKDIR /app/implementations/go

COPY implementations/go/ ./
COPY test-vectors/ /app/test-vectors/

CMD ["sh", "-c", "make clean && make build && make test && make coverage && make docs"]

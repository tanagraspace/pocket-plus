FROM python:3.11-alpine

WORKDIR /app/implementations/python

COPY implementations/python/ ./
COPY test-vectors/ /app/test-vectors/

RUN pip install --no-cache-dir -e ".[dev]"

# Default: lint, type check, test with coverage and HTML reports, generate API docs
CMD ["sh", "-c", "\
    ruff format --check . && \
    ruff check . && \
    mypy pocketplus && \
    mkdir -p build/docs && \
    pytest -v --cov=pocketplus \
        --cov-report=html:build/docs/coverage \
        --cov-report=xml:build/docs/coverage.xml \
        --html=build/docs/tests/report.html --self-contained-html && \
    pdoc -d google pocketplus -o build/docs/api \
"]

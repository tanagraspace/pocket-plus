FROM python:3.11-alpine

WORKDIR /app/implementations/python

COPY implementations/python/ ./
COPY test-vectors/ /app/test-vectors/

RUN pip install --no-cache-dir -e ".[dev]"

CMD ["sh", "-c", "ruff format --check . && ruff check . && mypy pocketplus && pytest -v --cov=pocketplus"]

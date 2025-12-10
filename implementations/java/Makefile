.PHONY: all build test test-report test-cli coverage coverage-html docs api-docs fmt fmt-check checkstyle spotbugs check clean cli bench

BUILD_DIR = build
DOCS_DIR = $(BUILD_DIR)/docs

all: build

build:
	mvn package -DskipTests

cli: build
	@echo "JAR built at target/pocketplus-1.0.0.jar"
	@echo "Run with: java -jar target/pocketplus-1.0.0.jar"

test:
	mvn test

test-report:
	mkdir -p $(BUILD_DIR)/tests
	mvn test surefire-report:report-only site -DgenerateReports=false
	cp target/site/surefire-report.html $(BUILD_DIR)/tests/index.html
	cp -r target/site/css $(BUILD_DIR)/tests/ 2>/dev/null || true
	cp -r target/site/images $(BUILD_DIR)/tests/ 2>/dev/null || true
	@echo "Test report at $(BUILD_DIR)/tests/index.html"

test-cli: cli
	./tests/test_cli.sh

coverage:
	mkdir -p $(BUILD_DIR)/coverage
	mvn verify jacoco:report
	cp -r target/site/jacoco/* $(BUILD_DIR)/coverage/
	@echo "Coverage report at $(BUILD_DIR)/coverage/index.html"

coverage-html:
	mkdir -p $(DOCS_DIR)/coverage
	mvn verify jacoco:report
	cp -r target/site/jacoco/* $(DOCS_DIR)/coverage/

docs: coverage test-report api-docs

api-docs:
	@mkdir -p $(BUILD_DIR)/api
	@echo "Generating API documentation..."
	mvn javadoc:javadoc
	cp -r target/site/apidocs/* $(BUILD_DIR)/api/

fmt:
	mvn spotless:apply

fmt-check:
	mvn spotless:check

checkstyle:
	mvn checkstyle:check

spotbugs:
	mvn spotbugs:check

check: fmt-check checkstyle spotbugs test

clean:
	mvn clean
	rm -rf $(BUILD_DIR)/* 2>/dev/null || true

bench: build
	@java -cp target/classes space.tanagra.pocketplus.cli.Bench $(ITERATIONS)

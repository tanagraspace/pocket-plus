"""Pytest configuration and fixtures."""

import pytest


def pytest_addoption(parser):
    """Add custom command line options."""
    parser.addoption(
        "--fast",
        action="store_true",
        default=False,
        help="Skip slow tests (venus-express, housekeeping)",
    )


def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line("markers", "slow: marks tests as slow")


def pytest_collection_modifyitems(config, items):
    """Skip slow tests when --fast flag is used."""
    if config.getoption("--fast"):
        skip_slow = pytest.mark.skip(reason="skipped with --fast flag")
        for item in items:
            if "slow" in item.keywords:
                item.add_marker(skip_slow)

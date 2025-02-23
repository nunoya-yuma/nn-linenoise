import os
import pytest
from typing import Generator

from blabot.process_io import ProcessIO


@pytest.fixture
def example_cli() -> Generator[ProcessIO, None, None]:
    THIS_FILE_PATH = os.path.abspath(__file__)
    EXECUTABLE_FILE_PATH = os.path.join(
        os.path.dirname(THIS_FILE_PATH), "../../examples/build/nn_cli_sample")

    START_COMMAND = EXECUTABLE_FILE_PATH
    PROMPT = "> "
    NEW_LINE = "\r"
    assert os.path.exists(EXECUTABLE_FILE_PATH), (
        f"Executable file not found: {EXECUTABLE_FILE_PATH}"
    )
    example_cli = ProcessIO(START_COMMAND, PROMPT, NEW_LINE)

    example_cli.start()
    assert example_cli.wait_for_prompt(), "Failed to start"
    yield example_cli
    example_cli.stop()


def test_sample_command(example_cli: ProcessIO) -> None:
    assert example_cli.run_command("sample-status", "Sample status: 'Invalid'")

    assert example_cli.run_command(
        "sample-ctrl on", "Sample status changed to 'on'")
    assert example_cli.run_command("sample-status", "Sample status: 'on'")

    assert example_cli.run_command(
        "sample-ctrl off", "Sample status changed to 'off'")
    assert example_cli.run_command("sample-status", "Sample status: 'off'")

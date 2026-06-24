#!/usr/bin/env python3

from __future__ import annotations

import argparse
import io
import subprocess
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest import mock

import expect_compile_failure


class ExpectCompileFailureTest(unittest.TestCase):
	def make_args(self, root: Path, expected_diagnostics: list[str]) -> argparse.Namespace:
		return argparse.Namespace(
			compiler="c++",
			standard="c++17",
			source=str(root / "sample.fail"),
			output=str(root / "build" / "sample.o"),
			include=[str(root / "include")],
			expect_diagnostic=expected_diagnostics,
		)

	def test_expected_diagnostic_in_stderr_is_accepted(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			args = self.make_args(
				Path(root_name),
				["integer member must be the exact fixed-width signedness type"],
			)
			completed = subprocess.CompletedProcess(
				args=["c++"],
				returncode=1,
				stdout="",
				stderr="static assertion failed: integer member must be the exact fixed-width signedness type\n",
			)

			with mock.patch("expect_compile_failure.subprocess.run", return_value=completed):
				status = expect_compile_failure.run(args)

			self.assertEqual(status, 0)

	def test_expected_diagnostic_in_stdout_is_accepted(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			args = self.make_args(Path(root_name), ["EXPECTED_MARKER"])
			completed = subprocess.CompletedProcess(
				args=["c++"],
				returncode=1,
				stdout="compiler wrapper: EXPECTED_MARKER\n",
				stderr="",
			)

			with mock.patch("expect_compile_failure.subprocess.run", return_value=completed):
				status = expect_compile_failure.run(args)

			self.assertEqual(status, 0)

	def test_unexpected_compile_failure_is_rejected(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			args = self.make_args(Path(root_name), ["EXPECTED_MARKER"])
			completed = subprocess.CompletedProcess(
				args=["c++"],
				returncode=1,
				stdout="",
				stderr="fatal error: ket_wire.h: No such file or directory\n",
			)
			stderr = io.StringIO()

			with mock.patch("expect_compile_failure.subprocess.run", return_value=completed):
				with redirect_stderr(stderr):
					status = expect_compile_failure.run(args)

			self.assertEqual(status, 1)
			self.assertIn("compilation failed for an unexpected reason", stderr.getvalue())
			self.assertIn("EXPECTED_MARKER", stderr.getvalue())
			self.assertIn("fatal error: ket_wire.h", stderr.getvalue())

	def test_successful_compile_is_rejected(self) -> None:
		with tempfile.TemporaryDirectory() as root_name:
			args = self.make_args(Path(root_name), ["EXPECTED_MARKER"])
			completed = subprocess.CompletedProcess(
				args=["c++"],
				returncode=0,
				stdout="",
				stderr="",
			)
			stderr = io.StringIO()

			with mock.patch("expect_compile_failure.subprocess.run", return_value=completed):
				with redirect_stderr(stderr):
					status = expect_compile_failure.run(args)

			self.assertEqual(status, 1)
			self.assertIn("expected compilation failure, but succeeded", stderr.getvalue())


if __name__ == "__main__":
	unittest.main()

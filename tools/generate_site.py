#!/usr/bin/env python3

from __future__ import annotations

import sys

import ket_tooling
import site_reference


def main() -> int:
	try:
		site_reference.generate_site()
	except Exception as error:  # noqa: BLE001 - command reports validation and generation failures.
		print(error, file=sys.stderr)
		return 1

	print(f"Generated {site_reference.SITE_OUTPUT_DIR}/")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())

#!/usr/bin/env python3

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import ket_tooling
import site_reference


def main() -> int:
	try:
		model = site_reference.load_site_model()
		errors = site_reference.validate_model(model)
		if errors:
			for error in errors:
				print(error, file=sys.stderr)
			return 1

		with tempfile.TemporaryDirectory() as temp_name:
			generated = Path(temp_name) / "site"
			site_reference.generate_site(generated)
			committed = ket_tooling.ROOT / site_reference.SITE_OUTPUT_DIR
			errors = site_reference.compare_directories(generated, committed)
			errors.extend(site_reference.validate_html_links(committed))
			if errors:
				for error in errors:
					print(error, file=sys.stderr)
				print("Run python3 tools/generate_site.py.", file=sys.stderr)
				return 1
	except Exception as error:  # noqa: BLE001 - command reports validation and generation failures.
		print(error, file=sys.stderr)
		return 1

	print("Site check passed.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())

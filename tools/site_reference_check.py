#!/usr/bin/env python3

from __future__ import annotations

import re
from html.parser import HTMLParser
from pathlib import Path


def iter_files(root: Path) -> list[Path]:
	if not root.exists():
		return []
	return sorted(path for path in root.rglob("*") if path.is_file())


def compare_directories(expected: Path, actual: Path) -> list[str]:
	errors: list[str] = []
	expected_files = {path.relative_to(expected).as_posix(): path for path in iter_files(expected)}
	actual_files = {path.relative_to(actual).as_posix(): path for path in iter_files(actual)}

	for relative_path in sorted(expected_files.keys() - actual_files.keys()):
		errors.append(f"committed site is missing generated file {relative_path}.")
	for relative_path in sorted(actual_files.keys() - expected_files.keys()):
		errors.append(f"committed site has stale extra file {relative_path}.")
	for relative_path in sorted(expected_files.keys() & actual_files.keys()):
		if expected_files[relative_path].read_bytes() != actual_files[relative_path].read_bytes():
			errors.append(f"committed site differs from generated output: {relative_path}.")

	return errors


HREF_PATTERN = re.compile(r'href="([^"]+)"')


def validate_html_links(site_dir: Path) -> list[str]:
	errors: list[str] = []
	for path in iter_files(site_dir):
		if path.suffix != ".html":
			continue
		text = path.read_text(encoding="utf-8")
		for href in HREF_PATTERN.findall(text):
			if "${" in href:
				continue
			if not href or href.startswith("#") or ":" in href:
				continue
			target_part = href.split("#", 1)[0]
			if not target_part:
				continue
			target = (path.parent / target_part).resolve()
			try:
				target.relative_to(site_dir.resolve())
			except ValueError:
				errors.append(f"{path.relative_to(site_dir)} links outside site: {href}")
				continue
			if not target.exists():
				errors.append(f"{path.relative_to(site_dir)} has broken link: {href}")
	return errors


class _SiteStructureParser(HTMLParser):
	def __init__(self, relative_path: str) -> None:
		super().__init__()
		self.relative_path = relative_path
		self.errors: list[str] = []
		self.has_skip_link = False
		self.has_main = False
		self.has_active_nav = False
		self.has_search_input = False
		self.has_search_label = False
		self.table_scroll_depth = 0
		self.nav_depth = 0
		self.stack: list[tuple[str, bool, bool]] = []

	def handle_starttag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
		attrs = {name: value or "" for name, value in attrs_list}
		class_names = set(attrs.get("class", "").split())
		opens_table_scroll = "table-scroll" in class_names
		opens_nav = tag == "nav"

		self._record_common_tag_state(tag, attrs, class_names)
		if tag == "table" and self.table_scroll_depth == 0:
			self.errors.append(f"{self.relative_path} has table outside .table-scroll.")

		if opens_table_scroll:
			self.table_scroll_depth += 1
		if opens_nav:
			self.nav_depth += 1
		self.stack.append((tag, opens_table_scroll, opens_nav))

	def handle_startendtag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
		attrs = {name: value or "" for name, value in attrs_list}
		class_names = set(attrs.get("class", "").split())
		self._record_common_tag_state(tag, attrs, class_names)
		if tag == "table" and self.table_scroll_depth == 0:
			self.errors.append(f"{self.relative_path} has table outside .table-scroll.")

	def handle_endtag(self, tag: str) -> None:
		while self.stack:
			stack_tag, opened_table_scroll, opened_nav = self.stack.pop()
			if opened_table_scroll:
				self.table_scroll_depth -= 1
			if opened_nav:
				self.nav_depth -= 1
			if stack_tag == tag:
				break

	def _record_common_tag_state(
		self,
		tag: str,
		attrs: dict[str, str],
		class_names: set[str],
	) -> None:
		obsolete = class_names & {"matrix", "diagram", "result"}
		for class_name in sorted(obsolete):
			self.errors.append(f"{self.relative_path} uses obsolete .{class_name} class.")

		if tag == "a" and attrs.get("href") == "#main" and "skip-link" in class_names:
			self.has_skip_link = True
		if tag == "main" and attrs.get("id") == "main":
			self.has_main = True
		if tag == "a" and attrs.get("aria-current") == "page" and self.nav_depth > 0:
			self.has_active_nav = True
		if tag == "input" and attrs.get("id") == "search-input":
			self.has_search_input = True
		if tag == "label" and attrs.get("for") == "search-input":
			self.has_search_label = True

	def finalize(self) -> list[str]:
		if not self.has_skip_link:
			self.errors.append(f"{self.relative_path} is missing skip link to #main.")
		if not self.has_main:
			self.errors.append(f"{self.relative_path} is missing main#main.")
		if not self.has_active_nav:
			self.errors.append(f"{self.relative_path} is missing active nav aria-current.")
		if self.has_search_input and not self.has_search_label:
			self.errors.append(f"{self.relative_path} has search input without label.")
		return self.errors


def validate_html_structure(site_dir: Path) -> list[str]:
	errors: list[str] = []
	for path in iter_files(site_dir):
		if path.suffix != ".html":
			continue

		relative_path = path.relative_to(site_dir).as_posix()
		parser = _SiteStructureParser(relative_path)
		try:
			parser.feed(path.read_text(encoding="utf-8"))
			parser.close()
		except Exception as error:  # noqa: BLE001 - report malformed files as validation errors.
			errors.append(f"{relative_path} could not be parsed: {error}")
			continue
		errors.extend(parser.finalize())
	return errors

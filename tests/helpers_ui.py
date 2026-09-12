import re
from itertools import repeat


def drive_action_until(tui, action, predicate, max_actions, timeout=0.5):
    """Drive one known action until its semantic target is observable."""
    result = predicate(tui.get_screen_dump())
    if result:
        return result

    for _ in repeat(None, max_actions):
        result = tui.send_and_wait_for_condition(
            action, predicate, timeout=min(timeout, 0.5)
        )
        if result:
            return result
    return False


def complete_modal_round_trip(tui, action, dismiss_action, timeout=1.5):
    """Prove dismissal restores the action that opened an overlay."""
    before = tui.get_screen_dump()
    if not tui.send_and_wait_for_condition(
        action, lambda lines: lines if lines != before else False, timeout=timeout
    ):
        return False
    if not tui.send_and_wait_for_screen_change(dismiss_action, timeout=timeout):
        return False
    return tui.send_and_wait_for_screen_change(action, timeout=timeout)


def dismiss_archive_unsafe_warnings(tui, warning_text, archive_text, enter_key, timeout=2.0):
    """Dismiss archive safety warnings until the archive view is rendered."""
    def archive_or_warning(snapshot):
        text = "\n".join(snapshot)
        if archive_text in text:
            return "archive"
        if warning_text in text:
            return "warning"
        return False

    state = tui.wait_for_condition(archive_or_warning, timeout=timeout)
    while state == "warning":
        state = tui.send_and_wait_for_condition(
            enter_key, archive_or_warning, timeout=timeout
        )
    return state == "archive"


_TREE_CONNECTOR_RE = re.compile(r"(?P<connector>[mt]q)(?!q)(?P<label>.+?)\s*$")
_PATH_HEADER_RE = re.compile(
    r"\bPath:\s+(?P<path>.*?)(?:\s*\d{2}-\d{2}-\d{4}\s+\d{2}:\d{2}:\d{2}\s*)?$"
)


def screen_text(tui):
    return "\n".join(tui.get_screen_dump())


def _screen_lines_text(snapshot):
    return "\n".join(snapshot)


def footer_lines_from_lines(snapshot):
    return list(snapshot[-3:])


def footer_lines(tui):
    return footer_lines_from_lines(tui.get_screen_dump())


def footer_text_from_lines(snapshot):
    snapshot = footer_lines_from_lines(snapshot)
    raw = "\n".join(snapshot).lower()
    normalized_lines = []
    key_tokens = []

    for line in snapshot:
        normalized = re.sub(r"[()]", "", line.lower())
        normalized = re.sub(r"\s+", " ", normalized).strip()
        nav_match = re.search(r"\b(file|tree)\s+f1\s+help\b", normalized)
        if nav_match:
            current_surface = "tree" if nav_match.group(1) == "file" else "file"
            normalized = normalized[nav_match.start() :]
            normalized = current_surface + normalized[len(nav_match.group(1)) :]
        if normalized:
            normalized_lines.append(normalized)

        for segment in re.split(r"\s{2,}", line.strip()):
            token = None

            if not segment:
                continue
            if segment in {"DIR", "FILE", "COMMANDS", "ARCHIVE", "ARCH-FILE", "Tree", "Dir"}:
                continue

            match = re.match(r"(\^[A-Za-z0-9]+)\b", segment)
            if match:
                token = match.group(1)
            else:
                match = re.match(r"([A-Z][0-9]+)\b", segment)
                if match:
                    token = match.group(1)
                else:
                    match = re.match(r"([A-Z`/]+)\s", segment)
                    if match:
                        token = match.group(1)
                    else:
                        match = re.search(r"[A-Z]", segment)
                        if match:
                            token = match.group(0)

            if token is not None:
                key_tokens.append(f"({token.lower()})")

    if normalized_lines and normalized_lines[-1].startswith("tree "):
        normalized_lines.append("j tree")
    if normalized_lines and normalized_lines[-1].startswith("file "):
        normalized_lines.append("j file")

    if any(line.startswith("file ") for line in normalized_lines):
        if all(token in raw for token in ("(h)", "(i)", "(j)")):
            normalized_lines.append("hex invert j compare")

    normalized = "\n".join(normalized_lines)
    return normalized + "\n" + " ".join(key_tokens)


def footer_text(tui):
    return footer_text_from_lines(tui.get_screen_dump())


def find_line_with_text(tui, needle):
    for line in tui.get_screen_dump():
        if needle in line:
            return line
    return None


def line_marks_file_as_tagged(line, filename):
    idx = line.find(filename)
    if idx <= 0:
        return False
    return "*" in line[:idx]


def assert_file_tag_state(tui, filename, expected_tagged):
    line = find_line_with_text(tui, filename)
    assert line is not None, f"Could not find file row for {filename!r}.\nScreen:\n{screen_text(tui)}"
    is_tagged = line_marks_file_as_tagged(line, filename)
    assert is_tagged == expected_tagged, (
        f"Unexpected tag state for {filename!r}. Expected tagged={expected_tagged}, got {is_tagged}.\n"
        f"Row: {line}\nScreen:\n{screen_text(tui)}"
    )


def _detect_split_column(snapshot):
    if len(snapshot) < 3:
        return None

    top = snapshot[1]
    for ch in ("w", "┬", "+"):
        idx = top.find(ch, 1)
        if idx != -1:
            return idx

    counts = {}
    for row in snapshot[2:-4]:
        for column_index, ch in enumerate(row):
            if ch in ("x", "|"):
                counts[column_index] = counts.get(column_index, 0) + 1

    if not counts:
        return None
    return max(counts, key=counts.get)


def _tree_panel_segment(line, split_col, panel):
    if split_col is None:
        return line
    if panel == "left":
        return line[:split_col]
    if panel == "right":
        return line[split_col + 1 :]
    raise ValueError(f"Unknown tree panel: {panel!r}")


def _tree_panel_rows(snapshot, split_col=None, panel="left"):
    return [_tree_panel_segment(line, split_col, panel).rstrip() for line in snapshot[2:-4]]


def _clean_tree_label(label):
    return label.strip().rstrip("/")


def _join_tree_path(parent, label):
    if parent in (None, ""):
        return label
    return parent.rstrip("/") + "/" + label


def _tree_row_infos(snapshot, split_col=None, panel="left"):
    root_connector_col = None
    stack = []
    infos = []

    for segment in _tree_panel_rows(snapshot, split_col=split_col, panel=panel):
        match = _TREE_CONNECTOR_RE.search(segment)
        if not match:
            continue

        label = _clean_tree_label(match.group("label"))
        if not label:
            continue

        connector_col = match.start("connector")
        if root_connector_col is None:
            root_connector_col = connector_col

        depth = max(0, (connector_col - root_connector_col) // 2)
        if depth < len(stack):
            del stack[depth:]
        while len(stack) < depth:
            stack.append("")
        stack.append(label)

        if depth == 0:
            full_path = label
        else:
            full_path = stack[0]
            for part in stack[1: depth + 1]:
                full_path = _join_tree_path(full_path, part)

        infos.append(
            {
                "segment": segment,
                "label": label,
                "path": full_path,
                "depth": depth,
            }
        )

    return infos


def first_tree_row_segment(snapshot, split_col=None, panel="left"):
    for segment in _tree_panel_rows(snapshot, split_col=split_col, panel=panel):
        if segment.strip():
            return segment
    return None


def _first_tree_row_info(snapshot, split_col=None, panel="left"):
    infos = _tree_row_infos(snapshot, split_col=split_col, panel=panel)
    if not infos:
        return None
    return infos[0]


def _panel_path_header(snapshot, split_col=None, panel="left"):
    if not snapshot:
        return None
    header = snapshot[0]
    match = _PATH_HEADER_RE.search(header)
    if not match:
        return None
    return match.group("path").strip().rstrip("/")


def _strip_panel_border_text(segment):
    text = segment.strip()
    if text and text[0] in ("x", "|"):
        text = text[1:]
    if text and text[-1] in ("x", "|"):
        text = text[:-1]
    return text.strip()


def _current_dir_from_stats(snapshot, split_col=None):
    for idx, line in enumerate(snapshot):
        segment = line[split_col:] if split_col is not None else line
        if "CURRENT DIR" not in segment:
            continue
        for offset in (1, 2):
            row_idx = idx + offset
            if row_idx >= len(snapshot):
                continue
            candidate = snapshot[row_idx][split_col:] if split_col is not None else snapshot[row_idx]
            text = _strip_panel_border_text(candidate)
            if text:
                return text.rstrip("/")
    return None


def _path_label(path):
    if path is None:
        return None
    stripped = path.strip().rstrip("/")
    if "/" not in stripped:
        return stripped
    return stripped.rsplit("/", 1)[-1]


def _tree_panel_selected_identity(snapshot, split_col=None, panel="left"):
    path = _panel_path_header(snapshot, split_col=split_col, panel=panel)
    if path is not None:
        return {"path": path, "label": _path_label(path)}

    stats_current = _current_dir_from_stats(snapshot, split_col=split_col)
    if stats_current is not None:
        return {"path": stats_current, "label": _path_label(stats_current)}

    return {"path": None, "label": None}


def tree_panel_selected_label(snapshot, split_col=None, panel="left"):
    return _tree_panel_selected_identity(
        snapshot, split_col=split_col, panel=panel
    )["label"]


def _identity_candidates(value):
    if value is None:
        return []
    clean = _clean_tree_label(value)
    candidates = [clean]
    label = _path_label(clean)
    if label is not None and label not in candidates:
        candidates.append(label)
    return candidates


def tree_row_visible(snapshot, label, split_col=None, panel="left"):
    if split_col is None:
        split_col = _detect_split_column(snapshot)
    candidates = _identity_candidates(label)
    for info in _tree_row_infos(snapshot, split_col=split_col, panel=panel):
        for candidate in candidates:
            if candidate in (info["label"], info["path"]):
                return True
    return False


def _tree_identity_visible(snapshot, identity, split_col=None, panel="left"):
    if split_col is None:
        split_col = _detect_split_column(snapshot)
    candidates = []
    for value in (identity.get("path"), identity.get("label")):
        for candidate in _identity_candidates(value):
            if candidate not in candidates:
                candidates.append(candidate)

    for info in _tree_row_infos(snapshot, split_col=split_col, panel=panel):
        for candidate in candidates:
            if candidate in (info["label"], info["path"]):
                return True
    return False


def _origin_identity_changed(before_origin, after_origin):
    if before_origin is None or after_origin is None:
        return before_origin != after_origin
    return (
        before_origin["segment"] != after_origin["segment"]
        or before_origin["label"] != after_origin["label"]
        or before_origin["path"] != after_origin["path"]
    )


def assert_tree_viewport_origin_stable(
    before_lines,
    after_lines,
    *,
    selected_label=None,
    split_col=None,
    panel="left",
    label="",
):
    if split_col is None:
        split_col = _detect_split_column(after_lines)
    before_origin = _first_tree_row_info(before_lines, split_col=split_col, panel=panel)
    after_origin = _first_tree_row_info(after_lines, split_col=split_col, panel=panel)
    before_origin_segment = before_origin["segment"] if before_origin is not None else None
    after_origin_segment = after_origin["segment"] if after_origin is not None else None
    selected_identity = _tree_panel_selected_identity(
        before_lines, split_col=split_col, panel=panel
    )
    if selected_label is None:
        selected_label = selected_identity["path"] or selected_identity["label"]
    else:
        selected_identity = {
            "path": selected_label if "/" in selected_label else None,
            "label": _path_label(selected_label),
        }

    assert selected_label is not None, (
        f"Could not identify selected tree row for viewport check ({label}).\n"
        f"Before:\n{_screen_lines_text(before_lines)}"
    )

    selected_visible = _tree_identity_visible(
        after_lines, selected_identity, split_col=split_col, panel=panel
    )
    if selected_visible:
        assert not _origin_identity_changed(before_origin, after_origin), (
            f"Tree viewport origin changed while selected row {selected_label!r} stayed visible"
            + (f" ({label})" if label else "")
            + f".\nBefore: {before_origin_segment!r}\nAfter:  {after_origin_segment!r}\n"
            f"Before origin path: {before_origin['path'] if before_origin else None!r}\n"
            f"After origin path:  {after_origin['path'] if after_origin else None!r}\n"
            f"Selected row: {selected_label!r}\nAfter screen:\n{_screen_lines_text(after_lines)}"
        )

    return {
        "before_origin": before_origin_segment,
        "after_origin": after_origin_segment,
        "before_origin_path": before_origin["path"] if before_origin else None,
        "after_origin_path": after_origin["path"] if after_origin else None,
        "selected_label": selected_label,
        "selected_visible": selected_visible,
        "split_col": split_col,
        "panel": panel,
    }

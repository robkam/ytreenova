import pexpect
import pyte
import time
import os
import signal
import tempfile
from ytnova_keys import Keys

class YtreeNovaController:
    def __init__(self, binary_path, cwd):
        self.time_scale = self._read_time_scale()
        log_target = os.environ.get("YTNOVA_TUI_DEBUG_LOG")
        if log_target:
            if log_target.lower() in {"1", "true", "yes"}:
                log_target = os.path.join(tempfile.gettempdir(), "ytnova_tui_debug.log")
            self.log_file = open(log_target, "w", encoding="utf-8")
        else:
            self.log_file = open(os.devnull, "w", encoding="utf-8")
        self.child = pexpect.spawn(
            binary_path,
            cwd=cwd,
            dimensions=(24, 160),
            encoding='utf-8',
            env={'TERM': 'xterm', 'LC_ALL': 'C.UTF-8', 'HOME': cwd},
            timeout=max(5.0 * self.time_scale, 5.0),
        )
        self.child.logfile = self.log_file
        self.screen = pyte.Screen(160, 24)
        self.stream = pyte.Stream(self.screen)
        self.last_wait_diagnostic = None

    def __del__(self):
        if hasattr(self, 'log_file'):
            self.log_file.close()

    @staticmethod
    def _read_time_scale():
        raw = os.getenv("YTNOVA_TUI_TIME_SCALE", "1.0")
        try:
            scale = float(raw)
        except (TypeError, ValueError):
            return 1.0
        return max(1.0, scale)

    def _scaled(self, seconds):
        return seconds * self.time_scale

    def _mirror_child_buffers(self, data):
        # Keep pexpect's expect() buffer in sync with screen-poll reads so
        # legacy controller helpers can mix both synchronization styles.
        self.child._before.write(data)
        self.child._buffer.write(data)

    def _read_output(self, timeout=0.1):
        try:
            read_timeout = self._scaled(timeout)
            while True:
                data = self.child.read_nonblocking(size=4096, timeout=read_timeout)
                self.stream.feed(data)
                self._mirror_child_buffers(data)
                read_timeout = 0
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def peek_screen_dump(self):
        return [str(line) for line in self.screen.display]

    def get_screen_dump(self):
        self._read_output(timeout=0.05)
        return self.peek_screen_dump()

    def wait_for_condition(self, predicate, timeout=5.0, poll_interval=0.02, description=None):
        deadline = time.monotonic() + self._scaled(timeout)

        while True:
            remaining = max(0.0, deadline - time.monotonic())
            self._read_output(timeout=min(self._scaled(poll_interval), remaining))
            lines = self.peek_screen_dump()
            result = predicate(lines)
            if result:
                self.last_wait_diagnostic = None
                return result
            if time.monotonic() >= deadline:
                label = description or getattr(predicate, "__name__", "screen predicate")
                self.last_wait_diagnostic = (
                    f"Timed out waiting for {label}; screen={lines!r}"
                )
                return False

    def wait_for_text(self, target, timeout=5.0, poll_interval=0.02):
        return self.wait_for_condition(
            lambda lines: lines
            if any(target in line for line in lines)
            else False,
            timeout=timeout,
            poll_interval=poll_interval,
            description=f"text {target!r}",
        )

    def send_and_wait_for_condition(
        self, keys, predicate, timeout=5.0, poll_interval=0.02
    ):
        self.child.send(keys)
        return self.wait_for_condition(
            predicate, timeout=timeout, poll_interval=poll_interval
        )

    def send_and_wait_for_screen_change(
        self, keys, timeout=5.0, poll_interval=0.02
    ):
        before = self.get_screen_dump()
        return self.send_and_wait_for_condition(
            keys,
            lambda lines: lines if lines != before else False,
            timeout=timeout,
            poll_interval=poll_interval,
        )

    def wait_for_startup(self):
        # Wait for first UI paint; do not couple startup sync to clock text.
        lines = self.wait_for_condition(
            lambda screen: screen
            if any("Path:" in line or "COMMANDS" in line for line in screen)
            else False,
            timeout=8.0,
        )
        if not lines:
            raise pexpect.TIMEOUT("Startup sync failed: no UI activity detected")

    def select_file(self, filename):
        """
        Selects a file using Show All + Filter.
        Remains in the Show All window so file commands work immediately.
        """
        # 0. Expand tree to ensure deep files are visible.
        self.send_and_wait_for_screen_change(Keys.EXPAND_ALL, timeout=1.0)

        # 1. Flatten tree to find nested files
        self.send_and_wait_for_screen_change(Keys.SHOWALL, timeout=1.0)

        # 2. Filter for the specific file
        lines = self.send_and_wait_for_condition(
            Keys.FILTER,
            lambda screen: screen
            if any("FILTER" in line for line in screen)
            else False,
            timeout=1.0,
        )
        if not lines:
            raise pexpect.TIMEOUT("Filter prompt did not appear")
        self.input_text(filename)

        # 3. Verify file is visible (do NOT press Enter)
        if not self.wait_for_text(filename, timeout=2.0):
            raise pexpect.TIMEOUT(f"Filtered file {filename!r} did not appear")

    def input_text(self, text):
        """Clears line with C-u and types text."""
        # Use C-u (\x15) instead of C-k (\x0b) because UI_ReadString
        # starts with the cursor at the end of the line.
        self.child.send("\x15")
        self._read_output(timeout=0.0)
        self.send_and_wait_for_screen_change(text + Keys.ENTER, timeout=1.5)

    def wait_for_refresh(self):
        """Waits for a screen update without depending on clock redraw text."""
        before = self.get_screen_dump()
        lines = self.wait_for_condition(
            lambda screen: screen if screen != before else False,
            timeout=2.0,
        )
        if not lines:
            raise pexpect.TIMEOUT("Refresh sync failed: no UI repaint detected")
        return lines

    def quit(self):
        """Aggressive quit."""
        try:
            if not self.child.isalive():
                return

            self.child.send(Keys.QUIT)
            try:
                self.child.expect(pexpect.EOF, timeout=0.5)
            except pexpect.TIMEOUT:
                pass

            if not self.child.isalive():
                return

            self.child.send(Keys.CONFIRM_YES)
            try:
                self.child.expect(pexpect.EOF, timeout=0.5)
            except pexpect.TIMEOUT:
                pass

            if self.child.isalive():
                try:
                    os.kill(self.child.pid, signal.SIGKILL)
                    os.waitpid(self.child.pid, 0)
                except OSError:
                    pass
        except Exception:
            # Ignore errors during teardown (e.g. process already dead, PtyProcessError)
            pass
        finally:
            try:
                self.child.close(force=True)
            except Exception:
                pass

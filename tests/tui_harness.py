import pexpect
import pyte
import time
import os

class YtreeNovaTUI:
    def __init__(
        self,
        executable="./build/ytnova",
        cwd=None,
        env_extra=None,
        args=None,
        dimensions=(36, 120),
    ):
        self.time_scale = self._read_time_scale()
        rows, cols = dimensions
        env = {
            "TERM": "xterm",
            "LC_ALL": "C.UTF-8",
            "HOME": cwd if cwd else "/tmp",
        }
        if env_extra:
            env.update(env_extra)
        
        # Launch ytnova in a headless PTY with specific dimensions
        self.child = pexpect.spawn(
            executable,
            args=args or [],
            env=env,
            dimensions=(rows, cols),
            cwd=cwd,
            encoding='utf-8',
            timeout=max(5.0 * self.time_scale, 5.0)
        )
        
        # Initialize an in-memory terminal screen using pyte
        self.screen = pyte.Screen(cols, rows)
        self.stream = pyte.Stream(self.screen)
        
        self.last_wait_diagnostic = None

        # Wait for the main UI tree to be ready (handles startup scan + any error dialogs)
        # The tree pane shows box-drawing like "tq" or "mq" once the dir is scanned.
        if not self.wait_for_content("tq", timeout=8.0) and not self.wait_for_content("mq", timeout=1.0):
            self._read_output(timeout=2.0)

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

    def _read_output(self, timeout=0.1):
        """Read pending output from the PTY and feed it to the virtual screen."""
        try:
            # Wait for the first PTY event, then drain everything already available.
            # pexpect owns the wait; tests never synchronize by wall-clock sleep.
            read_timeout = self._scaled(timeout)
            while True:
                data = self.child.read_nonblocking(size=4096, timeout=read_timeout)
                self.stream.feed(data)
                read_timeout = 0
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def send_keystroke(self, keys, wait=0.3):
        """Sends keys to the pexpect process, reads output, and updates screen."""
        self.child.send(keys)
        self._read_output(timeout=wait)

    def peek_screen_dump(self):
        """Returns an immutable snapshot of the current screen without draining the PTY."""
        return [str(line) for line in self.screen.display]

    def get_screen_dump(self):
        """Returns the screen display as a list of strings representing the grid."""
        # Ensure latest output is collected
        self._read_output(timeout=0.05)
        return self.peek_screen_dump()

    def wait_for_condition(self, predicate, timeout=5.0, poll_interval=0.02, description=None):
        """Poll until predicate(screen_lines) returns a truthy value or timeout expires."""
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
        """Wait until the target string appears anywhere on the screen."""
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
        """Send keys, then poll until predicate(screen_lines) becomes truthy."""
        self.child.send(keys)
        return self.wait_for_condition(
            predicate, timeout=timeout, poll_interval=poll_interval
        )

    def send_and_wait_for_screen_change(
        self, keys, timeout=5.0, poll_interval=0.02
    ):
        """Send keys, then wait for the rendered screen snapshot to change."""
        before = self.get_screen_dump()
        return self.send_and_wait_for_condition(
            keys,
            lambda lines: lines if lines != before else False,
            timeout=timeout,
            poll_interval=poll_interval,
        )

    def wait_for_content(self, target, timeout=5.0):
        """Wait until the target string appearing anywhere on the screen."""
        return bool(self.wait_for_text(target, timeout=timeout, poll_interval=0.1))

    def wait_for_exit(self, timeout=5.0):
        """Wait for the application to complete its orderly shutdown."""
        try:
            self.child.expect(pexpect.EOF, timeout=self._scaled(timeout))
        except pexpect.TIMEOUT:
            return False
        self._read_output(timeout=0)
        return True

    def quit(self):
        """Cleanly exit."""
        self.child.close(force=True)

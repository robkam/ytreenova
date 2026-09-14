from tui_harness import YtreeNovaTUI


def test_startup_readiness_rejects_scanning_notice_box_fragments():
    scanning_screen = [
        "lqqqqk",
        "x Scanning... x",
        "tqqqqu",
        "mqqqqj",
    ]

    assert not YtreeNovaTUI._startup_screen_ready(scanning_screen)
    assert not YtreeNovaTUI._startup_screen_ready(["tqqqqu", "mqqqqj"])
    assert YtreeNovaTUI._startup_screen_ready(["Path: /tmp", "COMMANDS"])

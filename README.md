# Grain

A quiet, minimalist idle game for Sailfish OS.

Build up a little park, one attraction at a time. Tap for your first coins, hire managers,
watch the numbers climb while you're away. Tend the aviary and small visitors will settle in.
No ads, no network, no account — just satisfying accumulation, offline, on your phone.

## Features

- Classic idle loop: tap → buy → automate → let it run
- Offline progress: the park keeps earning while the app is closed
- A calm cover with quick gestures — glance and go
- Detailed stats for optimizers (income breakdown per attraction)
- French and English
- Fully offline, no permissions, no telemetry

## Install

Grab the RPM for your device from the
[releases page](https://github.com/nicosouv/harbour-grain/releases):

- **armv7hl** — Jolla 1, Jolla C, Xperia X, Xperia XA2
- **aarch64** — Xperia 10 / 10 II / 10 III / 10 IV
- **i486** — Sailfish OS emulator

```bash
devel-su
pkcon install-local harbour-grain-*.rpm
```

Or tap the RPM in File Browser.

## Build

Built on GitHub Actions with the Sailfish SDK (see `.github/workflows`). Engine unit tests run
against plain Qt5:

```bash
cd tests
qmake tests.pro && make && ./tst_grain
```

## License

MIT — see [LICENSE](LICENSE).

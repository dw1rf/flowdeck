# Contributing to FlowDeck

Thanks for your interest! A few ground rules:

## License

By contributing, you agree that your contributions to the FlowDeck core will be licensed under the **PolyForm Noncommercial License 1.0.0**.

Plugins are separate — your plugin code can be under any license you choose.

## Plugin Contributions

- Plugins go in `plugins/` as a folder with a `manifest.json`.
- Plugins must not phone home or collect telemetry without explicit user consent.
- Document your hotkeys and commands in the manifest.

## Code Style

- Rust core: `cargo fmt`, `cargo clippy` clean.
- Python plugins: PEP 8, type hints where possible.
- Keep the core small. Push features into plugins.

## PRs

1. Fork the repo.
2. Create a branch: `feat/your-feature`.
3. Open a PR with a clear description.
4. Be nice in review.

## Issues

Found a bug? Open an issue with:
- OS version
- Steps to reproduce
- Expected vs actual behavior
- Logs (if any)

# Security policy

## Supported versions

Only the latest release and the current `master` branch get security fixes.

## Reporting a vulnerability

**Do not open a public issue for security problems.**

Report them privately through [GitHub's private vulnerability reporting](https://github.com/ksdevla/RetroStationX/security/advisories/new). Include:

- a description of the issue and its impact
- steps to reproduce, or a proof-of-concept file (for example a crafted ROM, patch, save state, movie, or Lua script)
- the affected version or commit, and your OS

We aim to reply within 7 days. Please give us a reasonable amount of time to release a fix before you disclose the issue publicly.

Areas where security matters most include the netplay server and client, file parsing (ROMs, archives, patches, save states, movies, HD packs), the Lua scripting sandbox, and the updater.

If the issue also affects MesenCE, please report it to the [MesenCE maintainers](https://github.com/nesdev-org/MesenCE) as well.

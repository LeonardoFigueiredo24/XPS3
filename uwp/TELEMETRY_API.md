# RPCS3 Xbox telemetry upload API (v1)

Base URL: `https://rpcs3.emelinhabibovic.de`

There is no secret: the app is open source. The server protects itself with
validation, size limits and rate limits. Anything not listed here gets 404.

## Identity

- `install_id`: random UUID v4 created on first launch, stored in the app's
  LocalState. Lowercase, with dashes, e.g. `3f2c9a1e-8b4d-4c7a-9e21-0d5b6f7a8c90`.
- `session_id`: random UUID v4 created each time the app process starts.
- No IP address, account, username, RPCN data or location is stored by the server.
  IPs are used in memory only for rate limiting.

## Common headers (both endpoints)

| Header | Required | Example |
| --- | --- | --- |
| `X-Telemetry-Install` | yes | install_id |
| `X-Telemetry-Session` | yes | session_id |
| `X-Telemetry-App-Version` | yes | `0.1.34.72` |
| `User-Agent` | no | `RPCS3Xbox/<version>`, e.g. `RPCS3Xbox/0.1.34.72` |

## POST /api/v1/events

`Content-Type: application/json`, body max 64 KB, max 50 events per request.

```json
{
  "events": [
    {
      "type": "app_start",
      "ts": "2026-10-24T18:00:00Z",
      "title_id": null,
      "title": null,
      "data": {}
    }
  ]
}
```

`type` is one of:

| type | when | useful `data` keys |
| --- | --- | --- |
| `install` | first launch after opting in (once) | `firmware` |
| `app_start` | every app start | `firmware`, `device` (e.g. "Xbox Series X"), `os_version`, `free_ram_mb` |
| `game_launch` | a game/XMB boots | `kind` (`game`, `homebrew`, `xmb`), `renderer`, `ppu_decoder`, `spu_decoder`, `resolution_scale` |
| `game_exit` | game stops normally | `duration_s`, `avg_fps` |
| `crash` | emulator crash or fatal error (also a crash found on next start) | `reason`, `exception_code`, `address`, `module`, `thread` |
| `fatal` | RPCS3 fatal error dialog/report | `message` |
| `perf` | optional periodic sample | `fps`, `ppu_pct`, `spu_pct` |
| `telemetry_disabled` | user turns telemetry off (last event sent) | — |

- `ts`: ISO 8601 UTC. Server clamps clearly wrong clocks.
- XMB boots: `title_id` null, `title` "XMB", `data.kind` "xmb".
- `title_id`: `^[A-Z]{4}\d{5}$` (e.g. `BLES01222`) or null. `title`: game name, max 128 chars.
- `data`: flat object, string/number/bool values, max 32 keys, strings max 1024 chars.
- Unknown `type` values are rejected per event (others in the batch are kept).

Response `200 {"ok":true,"accepted":N,"rejected":M}`.

## POST /api/v1/logs

Uploads one gzip-compressed log file. Body is the raw gzip bytes (do NOT set
`Content-Encoding`; the gzip is the payload itself).

`Content-Type: application/gzip`, body max 32 MB compressed, max 256 MB uncompressed.

Extra headers:

| Header | Required | Values |
| --- | --- | --- |
| `X-Telemetry-Log-Kind` | yes | `rpcs3` (RPCS3.log), `rpcs3_prev` (RPCS3.prev.log), `game` (RPCS3.game.log, last session that booted a game), `mesa`, `memory`, `crash` (crash dump text) |
| `X-Telemetry-Reason` | no | `crash`, `game_launch`, `game_exit`, `app_start`, `manual` |
| `X-Telemetry-Title-Id` | no | `BLES01222` |

Typical flow: on start, if the previous run crashed, upload `rpcs3_prev` with
reason `crash` and send a `crash` event. After a game exits, upload `rpcs3`
with reason `game_exit`.

Response `200 {"ok":true,"log_id":"..."}`.

The server also scrubs logs again (user folder names, emails, RPCN
password/token lines), but the app must not send that data in the first place.

## Errors and retry

| Status | Meaning | App should |
| --- | --- | --- |
| 400 | validation failed | drop, don't retry |
| 404 | wrong path/method | drop |
| 413 | too large | drop |
| 415 | wrong content type | drop |
| 429 | rate limited (`Retry-After` seconds) | keep queued, retry later |
| 5xx / network | server trouble | keep queued, retry with backoff |

Limits (defaults): per IP 120 requests / 10 min, per install 60 event
requests / 10 min and 20 log uploads / hour, 200 MB stored per install.

## Health

`GET /api/v1/health` → `200 {"ok":true}` (no data).

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
| `game_launch` | a game/XMB boots | `kind` (`game`, `homebrew`, `xmb`), `renderer`, `ppu_decoder`, `spu_decoder`, `resolution_scale`, `frame_limit`, `shader_mode`, `vblank_rate` |
| `game_exit` | game stops normally | `duration_s`, `avg_fps`, `min_fps` (worst 10 s window), `stutters` |
| `crash` | emulator crash or fatal error (also a crash found on next start) | `reason`, `exception_code`, `address`, `module`, `thread` |
| `fatal` | RPCS3 fatal error dialog/report | `message` |
| `perf` | every 10 s while a game/XMB runs (batched, sent once a minute) | see "Performance reports" |
| `stutter` | one long frame inside the last 10 s (at most 5 per 10 s, 300 per game run) | see "Performance reports" |
| `telemetry_disabled` | user turns telemetry off (last event sent) | — |

- `ts`: ISO 8601 UTC. Server clamps clearly wrong clocks.
- XMB boots: `title_id` null, `title` "XMB", `data.kind` "xmb".
- `title_id`: `^[A-Z]{4}\d{5}$` (e.g. `BLES01222`) or null. `title`: game name, max 128 chars.
- `data`: flat object, string/number/bool values, max 32 keys, strings max 1024 chars.
- Unknown `type` values are rejected per event (others in the batch are kept).

Response `200 {"ok":true,"accepted":N,"rejected":M}`.

### Performance reports

The app sends raw numbers only; the server decides what limited the frame rate
(so the rules can change without a new app build). Times are milliseconds,
percentages are "% of one CPU core" (a thread can't go over 100, totals can).
`t_s` is seconds since the game/XMB started, so samples line up on a graph.

`perf` (one 10 s window):

| key | meaning |
| --- | --- |
| `t_s`, `secs` | window end since launch; window length (about 10) |
| `fps`, `frames` | frames shown (flips) per second; count |
| `frame_max_ms`, `frames_50ms`, `frames_100ms`, `stutters` | longest frame; frames of 50+ / 100+ ms; `stutter` events in this window |
| `rsx_busy`, `rsx_idle` | RSX thread CPU time %, and % of time it had nothing to do (waiting for the game) |
| `gpu_wait_ms`, `submit_ms`, `present_ms` | per frame: waiting for the GPU (fences), submitting command buffers, waiting to present |
| `draws`, `submits`, `setup_ms`, `vertex_ms`, `texture_ms`, `draw_ms` | per frame: draw calls, submits, RSX time in draw setup, vertex upload, texture upload, draw execution |
| `shaders` | new pipelines (shader compiles) in the window |
| `ppu1`, `ppu1_pct`, `ppu2`, `ppu2_pct` | the two busiest PPU threads (game thread names, e.g. `main_thread`) and their load |
| `ppu_total`, `ppu_threads` | sum of all PPU thread loads; number of PPU threads |
| `spu_max`, `spu_total`, `spu_threads` | busiest SPU thread, sum, count |
| `mem_mb`, `mem_limit_mb` | app memory use and the console's limit for the app |

`stutter` (a frame of 100 ms or more and at least 2.5x the running average;
the longest 5 of each 10 s window):

| key | meaning |
| --- | --- |
| `t_s` | when (seconds since launch) |
| `frame_ms`, `avg_ms` | this frame; the running average before it |
| `shaders` | pipelines created during this frame |
| `submit_ms`, `present_ms`, `present_copy_ms`, `rsx_idle_ms`, `gpu_wait_ms` | where this frame's time went |
| `setup_ms`, `vertex_ms`, `texture_ms`, `draw_ms`, `draws`, `submits` | RSX work in this frame |
| `mem_poll_ago_ms` | how long before the frame's end the app last polled its memory (-1 unknown) |
| `rsx_phases` | the 1 ms RSX sampler for this frame, e.g. `RSX CPU 12 ms: fifo idle 61%; semaphore 22%;` (empty if not sampled) |
| `ppu1`, `ppu1_pct`, `spu_max`, `window_fps`, `mem_mb` | the surrounding 10 s window, for context |

## POST /api/v1/logs

Uploads one gzip-compressed log file. Body is the raw gzip bytes (do NOT set
`Content-Encoding`; the gzip is the payload itself).

`Content-Type: application/gzip`, body max 32 MB compressed, max 256 MB uncompressed.

Extra headers:

| Header | Required | Values |
| --- | --- | --- |
| `X-Telemetry-Log-Kind` | yes | `rpcs3` (RPCS3.log), `rpcs3_prev` (RPCS3.prev.log), `rpcs3_live` (a new part of RPCS3.log, see below), `game` (RPCS3.game.log, last session that booted a game), `mesa`, `memory`, `crash` (crash dump text) |
| `X-Telemetry-Reason` | no | `crash`, `game_launch`, `game_exit`, `app_start`, `manual`, `periodic` |
| `X-Telemetry-Log-Offset`, `X-Telemetry-Log-End` | for `rpcs3_live` | byte range of the part in the app's RPCS3.log (before scrubbing) |
| `X-Telemetry-Title-Id` | no | `BLES01222` |

Typical flow: on start, if the previous run crashed, upload `rpcs3_prev` with
reason `crash` and send a `crash` event. After a game exits, upload `rpcs3`
with reason `game_exit`.

Response `200 {"ok":true,"log_id":"..."}`.

### Live logs (`rpcs3_live`)

While a game/XMB runs, the app sends the part of RPCS3.log written since its last
part, once a minute (`X-Telemetry-Reason: periodic`, whole lines only), and the rest
at game exit or after a fatal error (`game_exit` / `crash`) instead of the whole log.
The server appends every part of an app start (session) to one stored log, so the
panel shows it as one file that grows.

- A part whose `End` is not past what the server has is a retry: `200` with `"duplicate":true`.
- A part that starts after what the server has gets a "bytes missing" line in front.
- At most 16 MB per part: when more was written, the app sends the newest 16 MB (a gap).
- The server owner can turn live logs off in the admin panel. The server then answers
  `403`; the app stops sending parts until it restarts, and at game exit uploads the
  whole log as `rpcs3` (as before) instead of the rest.
- Live parts have their own rate limit (default 90 / hour per install).

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
requests / 10 min, 20 log uploads / hour and 90 live log parts / hour, 200 MB
stored per install.

## Health

`GET /api/v1/health` → `200 {"ok":true}` (no data).

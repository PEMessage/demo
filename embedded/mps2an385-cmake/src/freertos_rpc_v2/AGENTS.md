# AGENTS.md — freertos_rpc_v2

## What this is

Extension of the stable single-message RPC (`rpc_call` / `rpc_dispatch`) to support **arbitrarily large messages** by fragmenting them across multiple `rpc_call` swaps.

## Stable core (do not change)

- `rpc_call(Func, Message*, Message*)` — client calls, blocks on FreeRTOS queue.
- `rpc_dispatch(void)` — server task dequeues and invokes the handler.
- `Message` / `MessageCreate` / `MessageDelete` / `MessageCopy` — stable heap helpers.

## Multi RPC extension (the mutable part)

### Protocol
- Every swap carries a fixed-size `Hdr` followed by payload.
- `Hdr` = `{magic(0xDEC0), SendHdr, RespHdr}`
- `SendHdr` is a tagged union driven by `SendType` (`SendInvalid / SendStart / SendUpdate / SendDone`):
  - `SendStart` carries `total` (expected payload bytes).
  - `SendUpdate` carries `offset` (where this fragment belongs in the reassembly buffer).
  - `SendDone` marks the end of the client-to-server stream.
- `RespHdr` is a tagged union driven by `RespType` (`RespInvalid / RespOK / RespErr`):
  - `RespOK` means the server accepted the fragment.
  - `RespErr` carries `errcode` (protocol-level failure).
  - The handler return value is embedded in the first 4 bytes of the reply payload, not in `RespHdr`.

### Example exchange (100-byte payload, `MESSAGE_MAX_LEN = 64`, `sizeof(Hdr) = 20`, fragment = 44 bytes)

> `sh` = `SendHdr`, `rh` = `RespHdr`.  Client-side `rh` stays `RespInvalid` during its own transmit phase because `CoRecv` is not started; Server-side `rh` stays `RespInvalid` during its reply phase because `CoRecv` has already reset and `CoSend` never touches `rh`.  The 4-byte handler return is prepended to the echo payload, so the reply `total` is 104 instead of 100.

| 轮次 | Client → Server (`sh` / `rh`) | Server → Client (`sh` / `rh`) |
|------|------------------------------|------------------------------|
| 1 | `SendStart(100)` / `RespInvalid` | `SendInvalid` / `RespOK` |
| 2 | `SendUpdate(0)` / `RespInvalid` | `SendInvalid` / `RespOK` |
| 3 | `SendUpdate(44)` / `RespInvalid` | `SendInvalid` / `RespOK` |
| 4 | `SendUpdate(88)` / `RespInvalid` | `SendInvalid` / `RespOK` |
| 5 | `SendDone` / `RespInvalid` | `SendStart(104)` / `RespOK` |
| 6 | `SendInvalid` / `RespOK` | `SendUpdate(0)` / `RespInvalid` |
| 7 | `SendInvalid` / `RespOK` | `SendUpdate(44)` / `RespInvalid` |
| 8 | `SendInvalid` / `RespOK` | `SendUpdate(88)` / `RespInvalid` |
| 9 | `SendInvalid` / `RespOK` | `SendDone` / `RespInvalid` |
| 10 | `SendInvalid` / `RespOK` | `SendInvalid` / `RespInvalid` |

### Invariants
1. One `rpc_call` = one swap; `rpc_call_multi` runs a `do { ... } while(CR_IS_STARTED(...))` loop over `StepMultiSession`.
2. Reassembly happens in a `MultiSession` object; the final payload is copied to `output` only after the entire exchange finishes.
3. Both sides use **coroutines** (`CR_START/YIELD/RESET/END`) to suspend between fragments; no explicit state machine.
   - Server-side: `CoRecv`, `OnEventServer` (the handler coroutine), `CoSend`.
   - Client-side: `OnEventClient` (drives the send/receive lifecycle).
4. `CoSend` and `CoRecv` do not allocate memory. `rx` / `tx` buffers are allocated dynamically by `OnEventServer` / `OnEventClient` when a session starts (via `MessageCreate`).
5. Error code base: `MULTI_ERR_BASE = -1100`. `0 = OK`, negative = protocol error, handler error = negative value (e.g. `-1200`).

### Key functions
- `multi_handler(Message*, Message*)` — server entry; holds a static `MultiSession` and calls `StepMultiSession`.
- `rpc_call_multi(Func, Message*, Message*)` — client entry; creates a temporary `MultiSession`, fragments input and reassembles output.
- `StepMultiSession(MultiSession*, Message*, Message*)` — validates magic + length, then dispatches `CoRecv` and `CoSend`. The handler coroutine (`OnEventServer` / `OnEventClient`) is invoked indirectly via `OnEvent` callbacks.

## Build / Run
```bash
make src/freertos_rpc_v2
make src/freertos_rpc_v2 run
```

## Testing tip
- `TaskClient` runs six tests per cycle:
  1. `100`-byte payload (table verification, expect `0`).
  2. `0`-byte payload (handler rejects empty input, expects `-1200`).
  3. `MULTI_CAP - 1` bytes (expect `0`).
  4. `MULTI_CAP` bytes (expect `0`).
  5. `MULTI_CAP + 1` bytes (server rejects as oversize, expects `MULTI_ERR_COSEND_START_ERR`).
  6. `output buffer = MULTI_CAP / 2` bytes (client rejects as response oversize, expects `MULTI_ERR_CORECV_NOMEM`).

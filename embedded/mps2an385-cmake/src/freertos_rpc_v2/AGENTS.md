# AGENTS.md — freertos_rpc

## What this is

Extension of the stable single-message RPC (`rpc_call` / `rpc_dispatch`) to support **arbitrarily large messages** by fragmenting them across multiple `rpc_call` swaps.

## Stable core (do not change)

- `rpc_call(Func, Message*, Message*)` — client calls, blocks on FreeRTOS queue.
- `rpc_dispatch(void)` — server task dequeues and invokes the handler.
- `Message` / `MessageCreate` / `MessageDelete` / `MessageCopy` — stable heap helpers.

## Multi RPC extension (the mutable part)

### Protocol
- Every swap carries a fixed-size header followed by payload.
- `SendHdr` = `{magic(0xDEC0), offset, total, handler}`
- `RecvHdr` = `{magic(0xDEC0), offset, total, status}`
- `total = UINT32_MAX` means "reply length not yet known".

### Invariants
1. One `rpc_call` = one swap; `rpc_call_multi` is a `while(tx < send_tot || rx < rx_need)` loop.
2. After every swap, any payload after `RecvHdr` is copied into `output`.
3. Server uses **coroutine** (`CR_START/YIELD/RESET/END`) to suspend between fragments; no explicit state machine.
4. `rx_buf` / `tx_buf` are allocated on first use and freed when the session ends (`CrCreateBuffer` / `CrDeleteBuffer`).
5. Error code base: `MULTI_ERR_BASE = -1100`. `0 = OK`, negative = protocol error, positive = handler error.

### Key functions
- `multi_handler(Message*, Message*)` — server entry; wraps `multi_handler_cr`.
- `rpc_call_multi(Func, Message*, Message*)` — client entry; fragments input and reassembles output.
- `parseSendHdr()` — validates magic + length before coroutine dispatch.

## Build / Run
```bash
make src/freertos_rpc
make src/freertos_rpc run
```

## Testing tip
- `TaskClient` currently requests `MessageCreate(1024)` bytes and expects the echo to match.
- Switch `MessageCreate(1025)` (exceeds `MULTI_CAP`) to verify oversize failure path.

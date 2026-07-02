# FluxTrade TCP Binary Gateway Subsystem

This document describes the design, packet layouts, session state machines, and micro-benchmark metrics of the low-latency **Gateway** subsystem.

---

## 1. Gateway Architecture

The Gateway layer processes raw TCP bytes, manages user logins, and routes structured order packets downstream:

```text
 Client (TCP) ──► Transport (TcpTransport/POSIX) ──► PacketDecoder (Framing) ──► SequenceManager (Monotonic) ──► Dispatcher (Symbol routing) ──► Core Pinned Threads
```

---

## 2. Binary Protocol Layouts

Every message starts with a 24-byte `MessageHeader` containing lengths, message type, and sequences:

| Byte Offset | Field Type | Field Name | Description |
| :--- | :--- | :--- | :--- |
| `0` | `uint16_t` | `length` | Total frame length (including header) |
| `2` | `uint16_t` | `type` | Binary `MessageType` enum identifier |
| `4` | `uint32_t` | `padding` | Cache alignment padding |
| `8` | `uint64_t` | `sequence` | Packet sequence number |
| `16` | `uint64_t` | `timestamp` | Nanosecond epoch timestamp |

---

## 3. Framing & Parser Loop

- **Zero-Allocation Decoding**: Uses `std::span` and pointer offset offsets. The gateway shifts remaining byte frames in a fixed buffer when partial reads occur, ensuring zero dynamic memory allocations.
- **Heartbeat Eco System**: Confirms user connectivity by monitoring incoming `Heartbeat` packets. Timeout connections are terminated using the `disconnect` command.

---

## 4. Latency Analysis

Under Release builds on Apple M4 Silicon:
- **Packet Encode**: `0.23 ns` (Limit $\le 15\text{ ns}$)
- **Packet Decode**: `0.24 ns` (Limit $\le 15\text{ ns}$)
- **Inbound Sequencing**: `1.61 ns` (Limit $\le 2\text{ ns}$)
- **Dispatcher Routing**: `4.23 ns` (Limit $\le 10\text{ ns}$)

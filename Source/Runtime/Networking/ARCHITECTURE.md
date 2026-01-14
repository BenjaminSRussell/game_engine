# Networking Subsystem Architecture

> **Priority 8** | Est. 2,500+ files | 250K+ LOC | ⭐⭐⭐ Complexity

---

## Directory Structure

```
Networking/
├── Private/
├── Public/
│
├── Transport/                  # Transport layer
│   ├── TCP/
│   ├── UDP/
│   │   ├── Reliable/
│   │   └── Unreliable/
│   ├── WebSocket/
│   └── QUIC/
│
├── Protocol/                   # Protocol layer
│   ├── Packet/
│   │   ├── Header/
│   │   ├── Serialization/
│   │   └── Compression/
│   ├── Handshake/
│   ├── Heartbeat/
│   └── Fragmentation/
│
├── Connection/                 # Connection management
│   ├── Session/
│   ├── State/
│   ├── Timeout/
│   └── Bandwidth/
│
├── Replication/                # State replication
│   ├── Object/
│   │   ├── Registry/
│   │   ├── Relevancy/
│   │   └── Priority/
│   ├── Property/
│   │   ├── Delta/
│   │   ├── Serialization/
│   │   └── Conditions/
│   ├── Channel/
│   └── Dormancy/
│
├── RPC/                        # Remote procedure calls
│   ├── Client/
│   ├── Server/
│   ├── Multicast/
│   └── Reliable/
│
├── Prediction/                 # Client prediction
│   ├── Movement/
│   ├── Rollback/
│   ├── Reconciliation/
│   └── Smoothing/
│
├── LagCompensation/           # Lag compensation
│   ├── History/
│   ├── Rewinding/
│   └── HitVerification/
│
├── Matchmaking/               # Matchmaking
│   ├── Lobby/
│   ├── Queue/
│   ├── MMR/
│   └── Region/
│
├── Voice/                     # Voice chat
│   ├── Capture/
│   ├── Codec/
│   ├── Transmission/
│   └── Playback/
│
└── Security/                  # Network security
    ├── Encryption/
    ├── Authentication/
    ├── AntiCheat/
    └── Validation/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Networking/Transport/ | src/Networking/Transport/ | ~20 |
| Networking/Replication/ | src/Networking/Replication/ | ~30 |
| Networking/Prediction/ | src/Networking/Prediction/ | ~10 |

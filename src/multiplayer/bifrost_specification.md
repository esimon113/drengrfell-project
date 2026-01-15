# Specification of the Bifrost Protocol

**Note:** This is a single-purpose protocol designed specifically for Drengrfell.

Regarding the name: In Norse mythology, "Bifrost" is the name of the bridge that connects Asgard with Midgard.
Therefore it's a fitting name for a protocol that connects clients to the server.


## 1. Overview

The general idea is to implement a client-server architecture, to avoid potential synchronization problems with a peer-to-peer approach. The server is the ultimate source of truth in the sense that only the server can mutate the gamestate. The clients send update requests to the server who validates them and takes according actions. The machine of the host of the multiplayer session also runs the Server. Regardless of this, all players are treated equally and are Clients that send requests to the server and receive responses.

Bifrost builds on the transmission control protocol (TCP), which provides a reliable transport layer. In future versions other transport layers may be used (i.e. QUIC), but for the forseeable future, TCP will remain the basis of bifrost.

This protocol does not provide sophisticated quality of services mechanisms. For some requests there is a result response sent by the server. But currently there is no specific acknowledgement per message. This might be added in another version of this protocol.

### 1.1 Architecture
```text
┌──────────────────────────────────────────────────────────────────────┐  
│                         HOST MACHINE                                 │  
├──────────────────────────────────────────────────────────────────────┤  
│  ┌─────────────────────┐       ┌─────────────────────────────────┐   │  
│  │   Game Client       │       │        GameServer               │   │  
│  │   (Host Player)     │◄─────►│  - GameSession                  │   │  
│  │                     │ TCP   │    ├─ GameState (authoritative) │   │  
│  │                     │ localhost  ├─ GameController            │   │  
│  │                     │       │    ├─ RNG (server-owned)        │   │  
│  └─────────────────────┘       │    └─ BuildingCosts config      │   │  
│                                └───────────────┬─────────────────┘   │  
└────────────────────────────────────────────────┼─────────────────────┘  
                                                 │ TCP (configurable port)
                    ┌────────────────────────────┼────────────────┐
            ┌───────▼───────┐            ┌───────▼───────┐       ...
            │ Remote Client │            │ Remote Client │    (2-6 players)
            └───────────────┘            └───────────────┘
```

### 1.2 Design Principles

- **Server Authority:** Server owns the authoritative gamestate *("single source of truth")*
- **Intention based commands:** Clients send what they want to do *(intention)*, not what happened
- **Full State Sync:** Complete GameState broadcast after every action. In future versions this might be changed to patrial state sync.
- **No Client Prediction:** Clients wait for server confirmation before updating the UI. This also might change in future versions, so that the client predicts the state (based on sent command) and preemtively updates the GUI, which might be revoked on decline of the command.


## 2. Message Header

All messages use length-prefix *("header")* over TCP. The prefix states how long the payload is and the payload contains all the other information. For the purpos of this game this is good enough, since there is a relatively low message frequency, and it also makes debugging easier (i.e. with wireshark).

- **Length:** 4 bytes *(uint32_t)* in network byte order *(=big-endian)*, represents payload size only
- **Payload:** utf-8 encoded JSON data
- **Max. message size:** 16 MB *(= 24bit)* -> prevent out-of-memory errors *(i.e. DoS attacks)*


## 3. Message Types

### 3.1 Common Fields

All messages must contain these fields in the json payload.
```json
{
	"type": "<MessageType>",
	"seq": 12345
}
```

- **type:** String identifier for the message type
- **seq:** Sequence number *(uint32_t)*, incremented per-client -> used to match requests with responses


### 3.2 Client → Server Messages

#### 3.2.1 Lobby Phase

| Type         | Description                         | Payload                                     |
| ------------ | ----------------------------------- | ------------------------------------------- |
| JoinRequest  | Client requests to join the session | `{ "playerName": "Viking1" }`                 |
| LeaveRequest | Client voluntarily leaves           | `{ }`                                         |
| ReadyToggle  | Toggle ready state                  | `{ "ready": true }`                           |
| UpdateConfig | Host updates world config           | `{ "config": { WorldGeneratorConfig JSON } }` |
| StartGame    | Host initiates game start           | `{ }`                                         |
| KickPlayer   | Host kicks a player                 | `{ "playerName": "Viking2" }`                 |

#### 3.2.2 Game Phase

| Type            | Description                  | Payload                      |
| --------------- | ---------------------------- | ---------------------------- |
| EndTurn         | End current turn             | `{ }`                          |
| BuildSettlement | Request to build settlement  | `{ "vertexId": 42 }`           |
| BuildRoad       | Request to build road        | `{ "edgeId": 17, "level": 1 }` |
| MoveHero        | Request hero movement        | `{ "targetTileId": 5 }`        |
| ...             | Future commands (trading...) | `{ ... }`                      |

#### 3.2.3 Connection management

| Type      | Description                        | Payload                        |
| --------- | ---------------------------------- | ------------------------------ |
| Ping      | Keep-alive / latency check         | `{ "timestamp": 1704067200000 }` |
| Reconnect | Attempt to rejoin after disconnect | `{ "playerName": "Viking1" }`    |


### 3.3 Server → Client Messages

#### 3.3.1 Connection and Lobby

| Type         | Description                    | Payload                                             |
| ------------ | ------------------------------ | --------------------------------------------------- |
| JoinResponse | Response to join request       | `{ "success": true, "playerId": 0, "error": null }` |
| LobbyState   | Broadcast current lobby state  | See [3.4.1](#341-lobbystate)                        |
| ConfigUpdate | Broadcast updated world config | `{ "config": { WorldGeneratorConfig JSON } }`       |
| Kicked       | Notify client they were kicked | `{ "reason": "Host kicked you" }`                   |

#### 3.3.2 Game

| Type         | Description                  | Payload                                                     |
| ------------ | ---------------------------- | ----------------------------------------------------------- |
| GameStarted  | Game has begun               | `{ "initialState": { GameState JSON } }`                    |
| GameState    | Full state sync              | `{ "state": { GameState JSON } }`                           |
| ActionResult | Response to a game action    | `{ "seq": 123, "success": true, "error": null }`            |
| GamePaused   | A player disconnected        | `{ "disconnectedPlayer": "Viking2", "timeoutSeconds": 60 }` |
| GameResumed  | Disconnected player returned | `{ "reconnectedPlayer": "Viking2" }`                        |
| GameOver     | Game ended                   | `{ "winnerId": 2, "scores": [...] }`                        |

#### 3.3.3 Connection

| Type              | Description                   | Payload                                                           |
| ----------------- | ----------------------------- | ----------------------------------------------------------------- |
| Pong              | Response to ping              | `{ "timestamp": 1704067200000, "serverTime": 1704067200005 }`     |
| ReconnectResponse | Response to reconnect attempt | `{ "success": true, "playerId": 1, "state": { GameState JSON } }` |
| ServerError       | Generic error                 | `{ "code": "INVALID_MESSAGE", "message": "..." }`                 |


### 3.4 Complex Payload Structures

#### 3.4.1 LobbyState

```json
{
	"type": "LobbyState",
	"seq": 0,
	"players": [
		{ "playerId": 0, "name": "Viking1", "ready": true, "isHost": true, "connected": true },
		{ "playerId": 1, "name": "Viking2", "ready": false, "isHost": false, "connected": true }
	],
	"hostId": 0,
	"config": {
		"version": 1,
		"columns": 24,
		"rows": 24,
		"generationMode": "PERLIN",
		"seed": 0,
		"reconnectTimeoutSeconds": 60
	},
	"minPlayers": 2,
	"maxPlayers": 6
}
```

#### 3.4.2 ActionResult
```json
{
	"type": "ActionResult",
	seq": 123,
	"success": false,
	"error": {
		"code": "INSUFFICIENT_RESOURCES",
		"message": "Not enough wood to build settlement"
	}
}
```


## 4. Session States
```text
┌─────────────┐     JoinRequest      ┌─────────────┐
│             │ ◄─────────────────── │             │
│   LOBBY     │                      │   Client    │
│             │ ───────────────────► │             │
└──────┬──────┘     LobbyState       └─────────────┘
       │ StartGame (host, all ready)
       ▼
┌─────────────┐
│   PLAYING   │ ◄───► Full GameState sync after each action
└──────┬──────┘
       │ Disconnect detected
       ▼
┌─────────────┐      GamePaused
│   PAUSED    │ ───────────────────► All clients
└──────┬──────┘
       ├─── Reconnect within timeout ──► PLAYING (GameResumed)
       └─── Timeout expires ──► Player kicked, PLAYING continues
       │ (from PLAYING)
       │ Win condition / host ends
       ▼
┌─────────────┐
│   ENDED     │───────────────────► GameOver to all
└─────────────┘
```

## 5. Protocol Errors

| Code                  | Description                                   |
|-----------------------|-----------------------------------------------|
| INVALID_MESSAGE       | Malformed JSON or unknown message type        |
| NOT_YOUR_TURN         | Action attempted out of turn                  |
| INVALID_ACTION        | Action not allowed in current phase           |
| INSUFFICIENT_RESOURCES| Not enough resources for building             |
| INVALID_LOCATION      | Cannot build at specified vertex/edge         |
| LOBBY_FULL            | Max players reached                           |
| NAME_TAKEN            | Player name already in use                    |
| NOT_HOST              | Action requires host privileges               |
| GAME_ALREADY_STARTED  | Cannot join, game in progress                 |
| NOT_ALL_READY         | Cannot start, not all players ready           |
| PLAYER_NOT_FOUND      | Referenced player doesn't exist               |


## 6. Building Costs (Server-Configured)

The server defines building costs in a configuration structure:
```json
{
	"settlementCost": {
		"FOREST": 1,
		"CLAY": 1,
		"GRASS": 1,
		"FIELD": 1
	},
	"roadCost": {
		"level1": {
			"FOREST": 1,
			"CLAY": 1
		},
		"level2": {
			"FOREST": 2,
			"CLAY": 2,
			"MOUNTAIN": 1
		}
	}
}
```
Clients never send costs, they're purely server-enforced. One future idea could be to make the costs configurable by the host.


## 7. Sequence Diagrams

### 7.1 Join Flow
```text
Client                          Server
  │                               │
  │──── JoinRequest ─────────────►│
  │     { playerName: "V1" }      │
  │                               │
  │◄─── JoinResponse ─────────────│
  │     { success, playerId: 0 }  │
  │                               │
  │◄─── LobbyState ───────────────│  (broadcast to all)
  │     { players: [...] }        │
```

### 7.2 Build Settlement Flow
```text
Client                          Server
  │                               │
  │──── BuildSettlement ─────────►│
  │     { seq: 5, vertexId: 42 }  │
  │                               │
  │                    ┌──────────┴──────────┐
  │                    │ Validate:           │
  │                    │ - Is it your turn?  │
  │                    │ - canBuildSettlement│
  │                    │ - hasEnoughResources│
  │                    └──────────┬──────────┘
  │                               │
  │◄─── ActionResult ─────────────│
  │     { seq: 5, success: true } │
  │                               │
  │◄─── GameState ────────────────│  (broadcast to all)
  │     { state: {...} }          │
```

### 7.3 Disconnect and Reconnect Flow
```text
Client1 (disconnects)           Server                    Client2
  │                               │                         │
  X (connection lost)             │                         │
  │                    ┌──────────┴──────────┐              │
  │                    │ Detect disconnect   │              │
  │                    │ Start 60s timer     │              │
  │                    └──────────┬──────────┘              │
  │                               │                         │
  │                               │──── GamePaused ────────►│
  │                               │     { player: "V1" }    │
  │                               │                         │
  │ (reconnects within timeout)   │                         │
  │                               │                         │
  │──── Reconnect ───────────────►│                         │
  │     { playerName: "V1" }      │                         │
  │                               │                         │
  │◄─── ReconnectResponse ────────│                         │
  │     { success, state }        │                         │
  │                               │                         │
  │                               │──── GameResumed ───────►│
  │                               │     { player: "V1" }    │
```

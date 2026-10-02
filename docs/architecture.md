# Architecture

> TODO: fill in once the architecture is decided. Keep it high level: subsystems and how they
> interact, not every class.

```mermaid
flowchart LR
    subgraph Client
        Input --> ClientNet[Networking]
        ClientNet --> Render[Rendering]
    end
    subgraph Server
        ServerNet[Networking] --> Logic[Game logic]
        Logic --> ServerNet
    end
    ClientNet <-- UDP --> ServerNet
```

## Subsystems

### Networking

### Game logic

### Rendering

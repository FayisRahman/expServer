# Stage 20: Load Balancing

In this stage, we added load balancing support for reverse proxy to distribute requests across multiple upstream servers.

## Load Balancing Strategies

- **`round_robin`** - Rotates through upstreams sequentially (0, 1, 2, 0, 1, 2...)
- **`ip_hash`** - Hashes client IP to consistently route same client to same upstream

## Modified Files

### `config/xps_config.h`
- Added fields to `xps_config_route_s`:
  - `load_balancing` (const char*) - Strategy name ("round_robin" or "ip_hash")
  - `_round_robin_counter` (u_long) - Internal counter for round-robin

### `config/xps_config.c`
- In `parse_server()`: Initialize default `load_balancing = "round_robin"` and `_round_robin_counter = 0`
- In `parse_route()`: Parse `load_balancing` from config JSON
- In `xps_config_lookup()`: Select upstream based on strategy:
  - **round_robin**: `index = counter++ % upstreams.length`
  - **ip_hash**: `index = inet_pton(client_ip) % upstreams.length`

## Config Example

```json
{
  "req_path": "/",
  "type": "reverse_proxy",
  "upstreams": [
    "localhost:3000",
    "localhost:3001",
    "localhost:3002"
  ],
  "load_balancing": "round_robin"
}
```

## Testing

```bash
# Round Robin - each request goes to next server
curl http://localhost:8002/
curl http://localhost:8002/
curl http://localhost:8002/

# IP Hash - same IP always goes to same server
curl --interface 127.0.0.1 http://localhost:8002/
curl --interface 127.0.0.2 http://localhost:8002/
```
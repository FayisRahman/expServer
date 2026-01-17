# Stage 18: IP Whitelisting and Blacklisting

In this stage, we added IP-based access control to allow or deny requests based on client IP addresses.

## Modified Files

### `config/xps_config.h`
- Added fields to `xps_config_route_s`:
  - `ip_whitelist` (vec_void_t)
  - `ip_blacklist` (vec_void_t)
- Added same fields to `xps_config_lookup_s`

### `config/xps_config.c`
- Added `vec_init(&route->ip_whitelist)` and `vec_init(&route->ip_blacklist)` in `parse_server()`
- Parse IP lists in `parse_route()`:
  - If only whitelist exists → use whitelist
  - If only blacklist exists → use blacklist
  - If both exist → whitelist takes priority (filter out blacklisted IPs from whitelist)
- Copy lists to lookup: `lookup->ip_whitelist = route->ip_whitelist`

### `core/xps_session.c`
In `session_process_request()`, added IP filtering logic:
1. Check if client IP is in whitelist (if whitelist exists)
2. Check if client IP is in blacklist (if blacklist exists)
3. Return `HTTP_FORBIDDEN` (403) if access denied


## Config Example

```json
{
  "ip_whitelist": ["127.0.0.1", "192.168.1.100"],
  "ip_blacklist": ["10.0.0.5"]
}
```

## Logic

- **Whitelist only**: Only listed IPs are allowed
- **Blacklist only**: Listed IPs are denied, all others allowed
- **Both**: Whitelist takes priority, blacklisted IPs removed from whitelist
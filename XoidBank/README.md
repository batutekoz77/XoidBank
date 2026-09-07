# XoidBank

XoidBank is a secure banking web application built from scratch in C++, featuring authentication, session management, two-factor verification, and Dutch banking compliance (BSN validation).

> ⚠️ **Status:** Active development. Backend authentication flow is functional; banking features (accounts, transactions, dashboard) are in progress.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Backend | C++ (Visual Studio 2026) |
| Web framework | [Crow](https://github.com/CrowCpp/Crow) |
| Async I/O | Asio (standalone) |
| Database | SQLite via [sqlite_modern_cpp](https://github.com/SqliteModernCpp/sqlite_modern_cpp) |
| Password hashing | [libsodium](https://libsodium.gitbook.io/doc/) (Argon2id) |
| Email delivery | libcurl (SMTP over Gmail) |
| Package management | vcpkg (manifest mode) |
| Frontend | Vanilla HTML / CSS / JS (per-page, served by the backend) |

---

## Features

### ✅ Implemented

- **Project architecture**
  - Modular page routing — every page under `Include/Pages/<PageName>/` is auto-discovered and served without manual route registration
  - Each feature owns its own router (`Application/<Feature>/<Feature>.hpp/.cpp`) to keep `Application.cpp` thin
  - Centralized `Crow.hpp` wrapper to manage the Crow/Asio/middleware include chain safely

- **User accounts**
  - Registration with full validation: name, email format, password confirmation, phone number, and **Dutch BSN (Burgerservicenummer)** validated with the official 11-proef algorithm
  - Unique constraints enforced at the database level for email, phone number, and BSN, each with a distinct, user-facing conflict message
  - Passwords hashed with Argon2id (libsodium) — never stored or logged in plaintext

- **Authentication flow**
  - Login → mandatory two-factor verification → authenticated session (2FA can never be skipped, even with "remember me" enabled)
  - 4-digit verification codes sent by email (Gmail SMTP via libcurl), valid for 120 seconds, with a maximum of 5 attempts
  - Three-tier cookie system:
    - `pending_session_id` — issued after a correct password, only grants access to the 2FA page
    - `session_id` — issued after successful 2FA, grants access to authenticated pages
    - `remember_token` — long-lived token that skips the password step on return visits, but **still routes through 2FA every time**
  - All cookies are `HttpOnly` + `SameSite=Strict`

- **Route protection**
  - Public pages (`Login`, `Register`) are always accessible
  - All other pages require a valid session and redirect to `/Login` automatically if missing
  - `/2FA` requires a valid pending session and redirects otherwise

- **Rate limiting**
  - Reusable `RateLimiter` skeleton (IP-based, thread-safe) supporting two patterns:
    - **Attempt-based limiting with penalty cooldown** (e.g. 5 attempts / 5 minutes → 10-minute lockout on the 6th)
    - **Simple cooldowns** for spam prevention on lightweight endpoints
  - Applied to `/Login/Submit`, `/Register/Submit`, and `/2FA/Verify`
  - Global flood protection middleware (e.g. 200 requests / 10 seconds per IP → 5-minute cooldown), applied to every route via Crow middleware

### 🚧 Planned

- Dashboard and account overview UI
- Bank accounts (checking/savings), balances stored as integer minor units (no floating-point money)
- Transactions (transfers, deposits, withdrawals) with status tracking
- Audit logging (login attempts, transfers, password changes)
- Account settings (password change, phone/email updates)
- Admin/support role tooling
- Config-driven deployment (moving hardcoded dev values into `Config.hpp`)

---

## Project Structure

```
XoidBank/
├── Application/
│   ├── Login/          # Login.hpp/.cpp — credential check, pending session creation
│   ├── Register/        # Register.hpp/.cpp — account creation, validation
│   ├── TwoFactor/       # TwoFactor.hpp/.cpp — code verification, session issuance
│   └── Application.hpp/.cpp  # Route registration, static page discovery, guards
├── Database/
│   └── Users.hpp/.cpp   # Users table, password hashing, uniqueness checks
├── Session/
│   └── Session.hpp/.cpp # Sessions, pending sessions, remember tokens
├── Include/
│   ├── Crow.hpp          # Central Crow/Asio/middleware include point
│   ├── CookieUtils.hpp   # Cookie parsing helper
│   ├── RateLimit/        # RateLimiter + GlobalRateLimitMiddleware
│   ├── Mailer/           # SMTP verification code delivery
│   └── Pages/<PageName>/ # Static HTML/CSS/JS per page
└── Main/
    ├── Main.cpp          # Entry point
    └── Config.hpp        # Environment-specific configuration (not tracked in detail here)
```

---

## Getting Started

### Prerequisites

- Visual Studio 2026 with the C++ workload
- [vcpkg](https://github.com/microsoft/vcpkg) installed and integrated (`vcpkg integrate install`)
- A Gmail account with an [App Password](https://support.google.com/accounts/answer/185833) for sending 2FA codes

### Dependencies (vcpkg manifest)

```json
{
  "dependencies": [
    "crow",
    "asio",
    "sqlite-modern-cpp",
    "libsodium",
    "curl"
  ]
}
```

Visual Studio will restore these automatically on build once manifest mode is enabled for the project.

### Configuration

`Main/Config.hpp` holds environment-specific values (host, port, database path, SMTP credentials) and is intentionally excluded from shared logic — set it up locally before running.

### Running

Build and run from Visual Studio. On first launch, the SQLite database is created automatically next to the executable.

---

## Security Notes

- Two-factor authentication cannot be bypassed, even with "remember me" — only the password step is skipped
- Money values will be stored as integers (minor currency units), never floats, once the accounts/transactions layer is implemented
- Sensitive input (passwords, verification codes) is never logged
- All authentication cookies are `HttpOnly` and `SameSite=Strict`

---

## License

TBD.

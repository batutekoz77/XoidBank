# 🏦 XoidBank

XoidBank is a secure banking web application built from scratch in C++. It handles authentication, mandatory two-factor verification, tiered session management, and Dutch banking compliance (BSN validation) — all served through a self-contained C++ backend with a per-page HTML/CSS/JS frontend.

> ⚠️ **Status:** Active development. Authentication, registration, and 2FA are fully functional. Banking features (dashboard, accounts, transactions) are in progress.

---

## 🛠 Features

- **No plaintext passwords, ever** — Argon2id hashing via libsodium
- **Mandatory two-factor authentication** — even "remember me" cannot skip the 2FA step, only the password step
- **Three-tier cookie/session system** — pending session → full session → remember token, each with its own scope and lifetime
- **Dutch BSN validation** — real 11-proef checksum algorithm, not just a length check
- **Unique-field conflict detection** — email, phone number, and BSN each report their own "already exists" error
- **Auto-discovered page routing** — drop HTML/CSS/JS into `Include/Pages/<PageName>/` and it's served automatically, no manual route wiring
- **Route guarding** — every page except Login/Register requires a valid session, unauthenticated visitors are redirected automatically
- **Rate limiting on every sensitive endpoint** — attempt-based limits with penalty cooldowns on Login, Register, and 2FA Verify
- **Global flood protection** — IP-based request-flood middleware applied to the entire app, independent of per-route limits
- **Email delivery for verification codes** — SMTP over Gmail via libcurl

---

## 🚀 Tech Stack

- **Language:** C++ (Visual Studio 2026)
- **Web framework:** [Crow](https://github.com/CrowCpp/Crow)
- **Async I/O:** Asio (standalone)
- **Database:** SQLite via [sqlite_modern_cpp](https://github.com/SqliteModernCpp/sqlite_modern_cpp)
- **Password hashing:** [libsodium](https://libsodium.gitbook.io/doc/) (Argon2id)
- **Email delivery:** libcurl (SMTP)
- **Package management:** vcpkg (manifest mode)
- **Target:** `x64/Release`

---

## 🎮 Roadmap

- [x] Auto-discovered static page routing (`Include/Pages/<PageName>/`)
- [x] Per-feature router architecture (`Application/<Feature>/`)
- [x] User registration with full validation (name, email, password, phone, BSN)
- [x] Dutch BSN validation (11-proef algorithm)
- [x] Unique constraint handling (email / phone / BSN conflict messages)
- [x] Argon2id password hashing
- [x] Login flow
- [x] Mandatory two-factor authentication (email-delivered codes)
- [x] Tiered session system (pending session / full session / remember token)
- [x] Route guards (redirect unauthenticated users to `/Login`)
- [x] Per-endpoint rate limiting with cooldown penalties
- [x] Global IP-based flood protection middleware
- [ ] Dashboard UI
- [ ] Bank accounts (checking / savings) with integer-based balances
- [ ] Transactions (transfers, deposits, withdrawals)
- [ ] Audit logging (login attempts, transfers, password changes)
- [ ] Account settings (password change, phone/email updates)
- [ ] Admin/support role tooling
- [ ] Production config separation (SMTP credentials, DB path, ports)

---

## 📁 Project Structure

```
XoidBank/
├── Application/
│   ├── Login/            # Login.hpp/.cpp — credential check, pending session creation
│   ├── Register/         # Register.hpp/.cpp — account creation, validation
│   ├── TwoFactor/        # TwoFactor.hpp/.cpp — code verification, session issuance
│   └── Application.hpp/.cpp  # Route registration, static page discovery, guards
├── Database/
│   └── Users.hpp/.cpp    # Users table, password hashing, uniqueness checks
├── Session/
│   └── Session.hpp/.cpp  # Sessions, pending sessions, remember tokens
├── Include/
│   ├── Crow.hpp           # Central Crow/Asio/middleware include point
│   ├── CookieUtils.hpp    # Cookie parsing helper
│   ├── RateLimit/         # RateLimiter + GlobalRateLimitMiddleware
│   ├── Mailer/            # SMTP verification code delivery
│   └── Pages/<PageName>/  # Static HTML/CSS/JS per page
└── Main/
    ├── Main.cpp           # Entry point
    └── Config.hpp         # Local configuration (host, port, DB path, SMTP credentials)
```

---

## Getting Started

### 1. Install Git

Download and install Git:
<https://git-scm.com/downloads>

### 2. Setup VCPKG & Dependencies

Open **CMD** and run:

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install crow:x64-windows
.\vcpkg install asio:x64-windows
.\vcpkg install sqlite-modern-cpp:x64-windows
.\vcpkg install libsodium:x64-windows
.\vcpkg install curl:x64-windows
.\vcpkg integrate install
```

Or, if the project uses manifest mode, just build once in Visual Studio and it will restore everything from `vcpkg.json` automatically.

### 3. Configure in Visual Studio

- Go to **Project → Properties**
- Select **Configuration Properties → vcpkg**
- Set **Use Vcpkg Manifest** to **Yes**

---

## ▶️ Running the Project

1. Open the project in **Visual Studio 2026**
2. Fill in `Main/Config.hpp` (database path, SMTP credentials for 2FA emails)
3. Make sure the configuration is set to **`x64` / `Release`**
4. Run with **F5** (Local Windows Debugger) — **do not** run the compiled `.exe` directly by double-clicking, it depends on the debugger's working directory to locate the `Include/Pages` folder correctly
5. Once running, open your browser at:

```
http://127.0.0.1:8080
```

> ⚠️ **Note:** Always start the server with **F5**, not by launching `XoidBank.exe` from `x64/Release` manually. The debugger sets the correct working directory for static file resolution.

---

## 🔐 Security Notes

- Two-factor authentication cannot be bypassed — "remember me" only skips the password step, never the verification code
- All authentication cookies (`session_id`, `pending_session_id`, `remember_token`) are `HttpOnly` and `SameSite=Strict`
- Verification codes expire after 120 seconds and are limited to 5 attempts per login
- Passwords are never logged or stored in plaintext
- Money values will be stored as integers (minor currency units) once the accounts/transactions layer is implemented — no floating-point balances

---

## License

TBD.

## Contributing

Pull requests are welcome! Feel free to open issues for bugs, feature requests, or ideas.

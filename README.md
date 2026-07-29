# Complaint Handling Management System (CHMS)

A modular, file-based Complaint Handling Management System written in
C11. No database is used — all persistence is via binary data files
(`users.dat`, `complaints.dat`, `notifications.dat`, `feedback.dat`).

## Build

### Linux / macOS
```
make
./cms
```

### Windows (VS Code + MinGW)
```
mingw32-make
cms.exe
```

### Manual build (any platform with GCC)
```
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 *.c -o cms
```

## Project Layout

| File              | Responsibility                                   |
|--------------------|---------------------------------------------------|
| `common.h`         | Shared macros, enums, structs                     |
| `utils.h/.c`       | Safe input, string handling, date/time, logging    |
| `security.h/.c`    | Password hashing, validation, session helpers      |
| `user.h/.c`        | Registration, login/logout, password management    |
| `complaint.h/.c`   | Complaint CRUD, search, category/priority/status   |
| `tracking.h/.c`    | Track a complaint by ID, expected resolution date  |
| `admin.h/.c`       | Admin login and admin menu                         |
| `report.h/.c`      | Summary/pending/resolved/category/priority/monthly reports, export |
| `notification.h/.c`| Notification creation and viewing                  |
| `feedback.h/.c`    | Post-resolution rating and comments                |
| `main.c`           | Top-level menu and program entry point             |

## Default Administrator Account

- Username: `admin`
- Password: `Admin@123`

Change `ADMIN_USERNAME` / `ADMIN_DEFAULT_PASSWORD` in `common.h` before
any real deployment.

## Data Files

`users.dat`, `complaints.dat`, `notifications.dat`, and `feedback.dat`
are created automatically on first write — they do not need to exist
beforehand. `audit.log` and `error.log` record audit events and
errors respectively, and are also created automatically.

## Security Notes

Passwords are never stored in plain text. A salted djb2-based digest
is used because no external crypto library is available in this
environment. **This is adequate for demonstrating the "no plaintext
passwords" secure-coding practice but is not a cryptographically
strong hash.** For production use, replace `hashPassword`/
`verifyPassword` in `security.c` with a vetted algorithm such as
bcrypt or Argon2.

## Known Limitations

- Single-admin account (hardcoded), no multi-admin management.
- Monthly report tracks up to 5 distinct calendar years.
- No concurrent-access locking on the data files (single-user, local
  use only).

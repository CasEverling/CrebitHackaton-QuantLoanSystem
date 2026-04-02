"""
plaid_bootstrap.py
==================
Ingests every available Plaid Sandbox persona into loanDatabase,
tagging all clients as college students (ACTIVITY_TYPE = 2) in Tampa.

Key decisions:
  - days_requested = 730  (Plaid maximum — ~2 years of history)
  - Institution: ins_109508 (First Platypus Bank — non-OAuth, all products)
  - All personas ingested as separate clients under one manager
  - Balance reconstruction: /accounts/get current balance as seed,
    walk backwards through /transactions/sync history
  - SyncState cursor persisted per access_token for the C++ daily loop

Usage:
    pip install requests psycopg2-binary
    export PLAID_CLIENT_ID=...
    export PLAID_SECRET=...
    python plaid_bootstrap.py

"""

import os
import hashlib
import logging
import datetime
import requests
import psycopg2

# ─────────────────────────────────────────────
# CONFIG
# ─────────────────────────────────────────────

PLAID_BASE_URL  = "https://sandbox.plaid.com"
PLAID_CLIENT_ID = os.getenv("PLAID_CLIENT_ID", "YOUR_CLIENT_ID")
PLAID_SECRET    = os.getenv("PLAID_SECRET",    "YOUR_SANDBOX_SECRET")

DB_DSN = os.getenv(
    "DB_DSN",
    "dbname=loanDatabase user=postgres password=sua_nova_senha host=127.0.0.1 port=5432"
)

# Non-OAuth institution — required for persona users and days_requested > 90
INSTITUTION_ID = "ins_109508"   # First Platypus Bank

# All available Plaid Sandbox personas.
# Each becomes a separate client in the DB.
# Password can be anything non-blank for persona users.
SANDBOX_PERSONAS = [
    {"override_username": "user_good",                 "override_password": "pass_good"},
    {"override_username": "user_transactions_dynamic", "override_password": "pass_good"},
    {"override_username": "user_ewa_user",             "override_password": "pass_good"},
    {"override_username": "user_yuppie",               "override_password": "pass_good"},
    {"override_username": "user_small_business",       "override_password": "pass_good"},
]

# Maximum history Plaid supports
DAYS_REQUESTED = 730

# All clients are tagged as college students
ACTIVITY_TYPE_STUDENT = 2

# Tampa location ID — deterministic hash of city|state
LOCATION_ID_TAMPA = int(
    hashlib.sha256("Tampa|FL".encode()).hexdigest(), 16
) % 100_000

# Single manager owns all ingested clients
MANAGER_SSN      = 100000000
MANAGER_USERNAME = "bootstrap_manager"
MANAGER_PASSWORD = "changeme"

# ─────────────────────────────────────────────
# Logging
# ─────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
log = logging.getLogger(__name__)


# ─────────────────────────────────────────────
# Plaid helpers
# ─────────────────────────────────────────────

def _plaid_post(endpoint: str, payload: dict) -> dict:
    url  = f"{PLAID_BASE_URL}{endpoint}"
    body = {"client_id": PLAID_CLIENT_ID, "secret": PLAID_SECRET, **payload}
    resp = requests.post(url, json=body, timeout=30)
    if not resp.ok:
        raise RuntimeError(
            f"Plaid {endpoint} failed {resp.status_code}: {resp.text[:400]}"
        )
    return resp.json()


def create_access_token(persona: dict) -> str:
    """
    Create a sandbox public token requesting 730 days of history,
    then exchange it for an access token.
    days_requested MUST be set here at link/token/create time —
    it cannot be extended after the Item is created.
    """
    username = persona["override_username"]
    log.info("Creating token for %s …", username)

    pt_resp = _plaid_post("/sandbox/public_token/create", {
        "institution_id":   INSTITUTION_ID,
        "initial_products": ["transactions", "identity"],
        "options": {
            "override_username": persona["override_username"],
            "override_password": persona["override_password"],
            "transactions": {
                "days_requested": DAYS_REQUESTED,
            },
        },
    })

    ex_resp = _plaid_post("/item/public_token/exchange", {
        "public_token": pt_resp["public_token"],
    })

    log.info("Access token obtained for %s.", username)
    return ex_resp["access_token"]


def get_accounts(access_token: str) -> dict:
    return _plaid_post("/accounts/get", {"access_token": access_token})


def drain_transactions_sync(access_token: str) -> tuple[list[dict], str]:
    """
    Drain all pages of /transactions/sync with no cursor
    to get the full 730-day history.
    Returns (all_transactions, final_cursor).
    """
    log.info("Draining /transactions/sync …")
    added    = []
    cursor   = None
    has_more = True

    while has_more:
        payload = {"access_token": access_token}
        if cursor:
            payload["cursor"] = cursor
        resp     = _plaid_post("/transactions/sync", payload)
        added   += resp.get("added", [])
        cursor   = resp.get("next_cursor", cursor)
        has_more = resp.get("has_more", False)

    log.info("Drained %d transactions.", len(added))
    return added, cursor


# ─────────────────────────────────────────────
# Derivation helpers
# ─────────────────────────────────────────────

def derive_client_ssn(persona_username: str) -> int:
    """Stable deterministic SSN from the persona username."""
    h = int(hashlib.sha256(persona_username.encode()).hexdigest(), 16)
    return 200_000_000 + (h % 700_000_000)


def reconstruct_daily_balances(
    transactions: list[dict],
    seed_date: datetime.date,
    seed_balance: float,
) -> dict[datetime.date, float]:
    """
    Starting from today's known balance, walk backwards through transactions
    to reconstruct an absolute balance for every date that has activity.

    Plaid sign convention: positive amount = money OUT (debit),
                           negative amount = money IN (credit).
    Walking backwards: add each debit back, subtract each credit.
    """
    # Sort descending — most recent first
    txns = sorted(transactions, key=lambda t: t["date"], reverse=True)

    snapshots: dict[datetime.date, float] = {seed_date: seed_balance}
    running = seed_balance

    for txn in txns:
        d      = datetime.date.fromisoformat(txn["date"])
        amount = txn.get("amount", 0.0)   # positive = debit in Plaid
        running += amount                  # reverse: recover prior balance
        if d not in snapshots:
            snapshots[d] = round(running, 2)

    return snapshots


# ─────────────────────────────────────────────
# DB
# ─────────────────────────────────────────────

class DB:
    def __init__(self, dsn: str):
        self.conn = psycopg2.connect(dsn)
        self.conn.autocommit = False

    def close(self):
        self.conn.close()

    def ensure_sync_state_table(self):
        with self.conn.cursor() as cur:
            cur.execute("""
                CREATE TABLE IF NOT EXISTS SyncState (
                    ACCESS_TOKEN TEXT PRIMARY KEY,
                    CURSOR       TEXT,
                    UPDATED_AT   TIMESTAMPTZ DEFAULT NOW()
                )
            """)
        self.conn.commit()

    def add_manager(self, ssn: int, username: str, password: str):
        with self.conn.cursor() as cur:
            cur.execute("""
                INSERT INTO Managers (SSN, USERNAME, PASSWORD)
                VALUES (%s, %s, %s)
                ON CONFLICT (SSN) DO NOTHING
            """, (ssn, username, password))
        self.conn.commit()

    def add_client(self, manager_ssn: int, client_ssn: int):
        with self.conn.cursor() as cur:
            cur.execute("""
                INSERT INTO Clients (MANAGER_SSN, CLIENT_SSN)
                VALUES (%s, %s)
                ON CONFLICT DO NOTHING
            """, (manager_ssn, client_ssn))
        self.conn.commit()

    def add_location(self, client_ssn: int, location_id: int, move_in_date: datetime.date):
        with self.conn.cursor() as cur:
            cur.execute("""
                INSERT INTO Location (CLIENT_SSN, LOCATION_ID, MOVE_IN_DATE)
                VALUES (%s, %s, %s)
                ON CONFLICT DO NOTHING
            """, (client_ssn, location_id, move_in_date))
        self.conn.commit()

    def add_freelancing_income(
        self,
        client_ssn: int,
        activity_type: int,
        start_date: datetime.date,
        end_date: datetime.date | None,
    ):
        with self.conn.cursor() as cur:
            cur.execute("""
                INSERT INTO FreelancingIncome (CLIENT_SSN, ACTIVITY_TYPE, START_DATE, END_DATE)
                VALUES (%s, %s, %s, %s)
                ON CONFLICT DO NOTHING
            """, (client_ssn, activity_type, start_date, end_date))
        self.conn.commit()

    def add_credit_information(
        self,
        client_ssn: int,
        date: datetime.date,
        balance: float,
    ):
        """
        Exact mirror of DataBaseManager::addCreditInformation.

        1. Insert absolute balance into AccountBalance.
        2. Fetch the previous snapshot.
        3. Compute delta / gap_days (handles missing days: weekends, etc.)
        4. Subtract any FixedIncome due today (students have none, so always 0).
        5. Insert variable delta into AccountBalanceVariableVariation.
        """
        with self.conn.cursor() as cur:

            # Step 1 — absolute balance
            cur.execute("""
                INSERT INTO AccountBalance (CLIENT_SSN, BALANCE, DATE)
                VALUES (%s, %s, %s)
                ON CONFLICT (CLIENT_SSN, DATE)
                    DO UPDATE SET BALANCE = EXCLUDED.BALANCE
            """, (client_ssn, balance, date))

            # Step 2 — previous snapshot
            cur.execute("""
                SELECT BALANCE, DATE
                FROM AccountBalance
                WHERE CLIENT_SSN = %s AND DATE < %s
                ORDER BY DATE DESC
                LIMIT 1
            """, (client_ssn, date))
            prev = cur.fetchone()

            if prev is None:
                # First row — seed only, no delta yet
                self.conn.commit()
                return

            prev_balance = float(prev[0])
            prev_date    = prev[1]
            gap_days     = max((date - prev_date).days, 1)

            # Step 3 — spread delta evenly across any gap
            delta = (balance - prev_balance) / gap_days

            # Step 4 — fixed income due today (none for students)
            cur.execute("""
                SELECT COALESCE(SUM(AMOUNT), 0)
                FROM FixedIncome
                WHERE CLIENT_SSN = %s
                  AND PAY_DAY    = %s
                  AND START_DATE <= %s
                  AND (END_DATE IS NULL OR END_DATE >= %s)
            """, (client_ssn, date.day, date, date))
            fixed_today    = float(cur.fetchone()[0])
            variable_delta = delta - fixed_today

            # Step 5 — variable delta
            cur.execute("""
                INSERT INTO AccountBalanceVariableVariation (CLIENT_SSN, BALANCE, DATE)
                VALUES (%s, %s, %s)
                ON CONFLICT (CLIENT_SSN, DATE)
                    DO UPDATE SET BALANCE = EXCLUDED.BALANCE
            """, (client_ssn, variable_delta, date))

        self.conn.commit()

    def save_cursor(self, access_token: str, cursor: str):
        with self.conn.cursor() as cur:
            cur.execute("""
                INSERT INTO SyncState (ACCESS_TOKEN, CURSOR, UPDATED_AT)
                VALUES (%s, %s, NOW())
                ON CONFLICT (ACCESS_TOKEN)
                    DO UPDATE SET CURSOR = EXCLUDED.CURSOR, UPDATED_AT = NOW()
            """, (access_token, cursor))
        self.conn.commit()


# ─────────────────────────────────────────────
# Per-persona ingestion
# ─────────────────────────────────────────────

def ingest_persona(db: DB, persona: dict):
    username   = persona["override_username"]
    today      = datetime.date.today()
    client_ssn = derive_client_ssn(username)

    log.info("── Ingesting %s  (client_ssn=%d) ──", username, client_ssn)

    access_token = create_access_token(persona)

    db.add_client(MANAGER_SSN, client_ssn)

    # Move-in date = start of the full history window
    earliest = today - datetime.timedelta(days=DAYS_REQUESTED)
    db.add_location(client_ssn, LOCATION_ID_TAMPA, earliest)

    # Tag as college student, currently enrolled
    db.add_freelancing_income(
        client_ssn,
        ACTIVITY_TYPE_STUDENT,
        earliest,
        None,
    )

    # Get current balance as reconstruction seed
    accounts_resp = get_accounts(access_token)
    accounts      = accounts_resp.get("accounts", [])

    # Prefer checking account; fall back to first available
    primary = next(
        (a for a in accounts if a.get("subtype") == "checking"),
        accounts[0] if accounts else None,
    )

    if primary is None:
        log.warning("No accounts found for %s — skipping.", username)
        return

    seed_balance = float(primary["balances"]["current"])
    log.info("Seed balance: $%.2f", seed_balance)

    transactions, cursor = drain_transactions_sync(access_token)

    daily_balances = reconstruct_daily_balances(transactions, today, seed_balance)
    log.info("Reconstructed %d daily snapshots over %d days.",
             len(daily_balances), DAYS_REQUESTED)

    # Insert in chronological order — each row needs a prior snapshot to diff against
    for date in sorted(daily_balances):
        db.add_credit_information(client_ssn, date, daily_balances[date])

    db.save_cursor(access_token, cursor)

    log.info(
        "✓ %s  snapshots=%d  range=%s → %s",
        username,
        len(daily_balances),
        min(daily_balances),
        max(daily_balances),
    )


# ─────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────

def main():
    log.info("=== Plaid Sandbox Bootstrap ===")
    log.info("Personas: %d  |  History: %d days  |  Institution: %s",
             len(SANDBOX_PERSONAS), DAYS_REQUESTED, INSTITUTION_ID)

    db = DB(DB_DSN)
    try:
        db.ensure_sync_state_table()
        db.add_manager(MANAGER_SSN, MANAGER_USERNAME, MANAGER_PASSWORD)

        for persona in SANDBOX_PERSONAS:
            try:
                ingest_persona(db, persona)
            except RuntimeError as e:
                log.error("Failed %s: %s", persona["override_username"], e)

        log.info("=== Done ===")
        log.info("All clients tagged: ACTIVITY_TYPE=%d (student)  LOCATION_ID=%d (Tampa FL)",
                 ACTIVITY_TYPE_STUDENT, LOCATION_ID_TAMPA)
    finally:
        db.close()


if __name__ == "__main__":
    main()

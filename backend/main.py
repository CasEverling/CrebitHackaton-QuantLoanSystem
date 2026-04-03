from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional
import numpy as np
import random
import math

app = FastAPI(title="Meridian API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── Mock client database ───────────────────────────────────────────────
CLIENTS = [
    {"id": "1", "name": "Carlos Silva", "occupation": "Uber Driver", "location": "São Paulo, SP", "incomeFixed": 0, "incomeVariable": 100, "bankConnected": True},
    {"id": "2", "name": "Ana Santos", "occupation": "iFood Delivery Driver", "location": "Rio de Janeiro, RJ", "incomeFixed": 20, "incomeVariable": 80, "bankConnected": True},
    {"id": "3", "name": "João Oliveira", "occupation": "Freelance Designer", "location": "Belo Horizonte, MG", "incomeFixed": 40, "incomeVariable": 60, "bankConnected": False},
    {"id": "4", "name": "Maria Costa", "occupation": "Freelance Developer", "location": "Curitiba, PR", "incomeFixed": 30, "incomeVariable": 70, "bankConnected": True},
    {"id": "5", "name": "Pedro Ferreira", "occupation": "99 Driver", "location": "Brasília, DF", "incomeFixed": 10, "incomeVariable": 90, "bankConnected": True},
    {"id": "6", "name": "Julia Almeida", "occupation": "Freelance Photographer", "location": "Porto Alegre, RS", "incomeFixed": 25, "incomeVariable": 75, "bankConnected": False},
]

class SimulateRequest(BaseModel):
    ssn: int
    loan_amount: float
    max_interest_rate: float
    pay_day: int
    min_profit: Optional[float] = 0

# ── GET /clients ──────────────────────────────────────────────────────
@app.get("/clients")
def get_clients():
    return CLIENTS

# ── POST /analyze ─────────────────────────────────────────────────────
@app.post("/analyze")
def analyze(req: SimulateRequest):
    loan_amount = req.loan_amount
    max_rate = req.max_interest_rate   # monthly rate, e.g. 0.15 = 15%
    pay_day = req.pay_day              # day of month
    num_steps = 8                      # 8 weeks
    num_paths = 100

    daily_rate = max_rate / 30.0

    # Monte Carlo paths
    paths: list[list[float]] = []
    for _ in range(num_paths):
        path = []
        profit = 0.0
        for step in range(num_steps):
            # Stochastic profit: daily interest * 7 days per step ± noise
            step_profit = loan_amount * daily_rate * 7 * (1 + random.gauss(0, 0.05))
            # Small default probability each step
            if random.random() < 0.01:
                step_profit = -loan_amount * 0.30
                path.append(round(profit + step_profit, 2))
                break
            profit += step_profit
            path.append(round(profit, 2))
        # Pad shorter paths (from early default) to num_steps length
        while len(path) < num_steps:
            path.append(path[-1] if path else 0.0)
        paths.append(path)

    final_profits = [p[-1] for p in paths]
    avg_profit = float(np.mean(final_profits))
    std_dev = float(np.std(final_profits))
    repayment_prob = float(sum(1 for p in final_profits if p > 0) / num_paths * 100)

    # Interest rate sweep
    sweep = []
    for rate_offset in [-2, -1, 0, 1, 2, 3, 4, 5]:
        r = max(0.001, max_rate + rate_offset * 0.01)
        dr = r / 30.0
        sweep_profits = []
        for _ in range(30):
            p = 0.0
            for _ in range(num_steps):
                p += loan_amount * dr * 7 * (1 + random.gauss(0, 0.05))
            sweep_profits.append(p)
        sweep.append({
            "interest_rate": round(r * 100, 2),  # as percentage
            "avg_profit": round(float(np.mean(sweep_profits)), 2),
            "repayment_probability": round(
                min(100, max(0, 95 - rate_offset * 3)), 2
            ),
        })

    recommended_rate = max_rate

    return {
        "request": {
            "ssn": req.ssn,
            "loan_amount": loan_amount,
            "max_interest_rate": max_rate,
            "pay_day": pay_day,
            "min_profit": req.min_profit,
        },
        "response": {
            "recommended_interest_rate": recommended_rate,
            "viable": avg_profit > 0,
            "paths": paths,
            "statistics": {
                "avg_profit": round(avg_profit, 2),
                "profit_std_dev": round(std_dev, 2),
                "repayment_probability": round(repayment_prob, 2),
                "max_profit": round(max(final_profits), 2),
                "min_profit": round(min(final_profits), 2),
                "confidence_interval": {
                    "lower": round(avg_profit - 1.96 * std_dev, 2),
                    "upper": round(avg_profit + 1.96 * std_dev, 2),
                    "confidence": 0.95,
                },
            },
            "interest_rate_sweep": sweep,
        },
    }

@app.get("/health")
def health():
    return {"status": "ok", "message": "Meridian API running"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)

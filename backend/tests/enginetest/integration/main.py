from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List

# Initialize the FastAPI app
app = FastAPI(title="QuantRisk Engine API (Mock)")

# ─────────────────────────────────────────────────────────────────────
# CORS Middleware (Crucial for connecting to your frontend)
# ─────────────────────────────────────────────────────────────────────
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Allows all origins
    allow_credentials=True,
    allow_methods=["*"],  # Allows all methods (GET, POST, OPTIONS, etc.)
    allow_headers=["*"],  # Allows all headers
)

# ─────────────────────────────────────────────────────────────────────
# Pydantic Models (Data Validation)
# ─────────────────────────────────────────────────────────────────────
class SimulateRequest(BaseModel):
    id: str
    amount: float
    min_interest_day: float

class SimulateResponseItem(BaseModel):
    interest_rate: float
    risk: float
    expected_profit: float

class SimulateResponse(BaseModel):
    data: List[SimulateResponseItem]

class ClientItem(BaseModel):
    id: str
    ssn: str
    name: str

class ClientsResponse(BaseModel):
    data: List[ClientItem]

# ─────────────────────────────────────────────────────────────────────
# Routes
# ─────────────────────────────────────────────────────────────────────

@app.get("/clients", response_model=ClientsResponse)
async def get_clients():
    """
    Returns a mock list of available clients for the demo.
    """
    mock_clients = [
        {"id": "1", "ssn": "826055747", "name": "Joao Silva"},
        {"id": "2", "ssn": "585420915", "name": "Maria Santos"},
        {"id": "3", "ssn": "173244838", "name": "Carlos Mendes"}
    ]
    return {"data": mock_clients}


@app.post("/simulate", response_model=SimulateResponse)
async def simulate_loan(request: SimulateRequest):
    """
    Mocks the Monte Carlo engine's response for the UI Demo.
    Generates an array of risk and profit metrics scaling with the interest rate.
    """
    results = []
    
    rate = request.min_interest_day
    max_rate = 0.01  # 1% daily maximum for the mock loop
    step = 0.002

    # Failsafe if frontend sends a weird number
    if rate > max_rate:
        raise HTTPException(status_code=400, detail="min_interest_day must be <= 0.01")

    # Generate the mock curve
    while rate <= max_rate:
        # Mock logic: Risk scales linearly for the sake of the demo visual
        mocked_risk = (rate * 100) / 2.0 
        
        # Compound interest calculation matching your C++ logic: Amount * (1 + rate)^90 - Amount
        mocked_profit = (request.amount * ((1.0 + rate) ** 90)) - request.amount
        
        results.append({
            "interest_rate": round(rate, 5),
            "risk": round(mocked_risk, 2),
            "expected_profit": round(mocked_profit, 2)
        })
        
        rate += step

    return {"data": results}


# ─────────────────────────────────────────────────────────────────────
# Server Execution
# ─────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    import uvicorn
    print("──── QuantRisk Engine API is LIVE ────")
    print("Test it at: http://localhost:8080/docs")
    uvicorn.run(app, host="0.0.0.0", port=8080)

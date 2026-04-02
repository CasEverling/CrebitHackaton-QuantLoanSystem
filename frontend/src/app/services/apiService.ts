const API_BASE = "http://localhost:8080";

export interface Client {
  id: string;
  name: string;
  occupation: string;
  location: string;
  incomeFixed: number;
  incomeVariable: number;
  bankConnected: boolean;
}

export interface SimulationDataPoint {
  name: string;
  avg: number;
  min: number;
  max: number;
}

export interface MonteCarloRequest {
  ssn: number;
  loan_amount: number;
  max_interest_rate: number;
  pay_day: number;
  min_profit: number;
}

export interface Statistics {
  avg_profit: number;
  profit_std_dev: number;
  repayment_probability: number;
  max_profit: number;
  min_profit: number;
  confidence_interval: {
    lower: number;
    upper: number;
    confidence: number;
  };
}

export interface InterestRateSweep {
  interest_rate: number;
  avg_profit: number;
  repayment_probability: number;
}

export interface MonteCarloResponse {
  recommended_interest_rate: number;
  viable: boolean;
  paths: number[][];
  statistics: Statistics;
  interest_rate_sweep: InterestRateSweep[];
}

export interface ApiResponse {
  request: MonteCarloRequest;
  response: MonteCarloResponse;
  simulationData: SimulationDataPoint[];
}

// ── fetch all clients for the banner list ─────────────────────────────
export async function fetchClients(): Promise<Client[]> {
  const res = await fetch(`${API_BASE}/clients`);
  if (!res.ok) throw new Error(`Failed to fetch clients: ${res.status}`);
  return res.json();
}

// ── run Monte Carlo simulation for a client ───────────────────────────
export async function simulateCredit(params: {
  clientId: string;
  loanAmount: number;
  maxInterestRate: number;
  payDay: number;        // day of month, e.g. 10
  minProfit: number;
}): Promise<ApiResponse> {
  const body: MonteCarloRequest = {
    ssn:               parseInt(params.clientId),
    loan_amount:       params.loanAmount,
    max_interest_rate: params.maxInterestRate,
    pay_day:           params.payDay,
    min_profit:        params.minProfit,
  };

  const res = await fetch(`${API_BASE}/analyze`, {
    method:  "POST",
    headers: { "Content-Type": "application/json" },
    body:    JSON.stringify(body),
  });

  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: res.statusText }));
    throw new Error(err.error ?? "Simulation failed");
  }

  const data = await res.json();

  // The backend wraps request + response together.
  // Build simulationData from the paths for the chart.
  const paths: number[][] = data.response?.paths ?? [];
  const simulationData: SimulationDataPoint[] = [];

  if (paths.length > 0) {
    const len = paths[0].length;
    for (let i = 0; i < len; i++) {
      const vals = paths.map(p => p[i]);
      simulationData.push({
        name: `T${i + 1}`,
        avg:  vals.reduce((a, b) => a + b, 0) / vals.length,
        min:  Math.min(...vals),
        max:  Math.max(...vals),
      });
    }
  }

  return { ...data, simulationData };
}

// Legacy wrapper so existing code using apiService.simulateCredit() still works
export const apiService = {
  simulateCredit: (formData: any) =>
    simulateCredit({
      clientId:         formData.clientId ?? String(formData.ssn ?? "0"),
      loanAmount:       formData.loanAmount ?? formData.loan_amount ?? 5000,
      maxInterestRate:  formData.maxInterestRate ?? formData.max_interest_rate ?? 0.15,
      payDay:           formData.payDay ?? formData.pay_day ?? 10,
      minProfit:        formData.minProfit ?? formData.min_profit ?? 0,
    }),
};

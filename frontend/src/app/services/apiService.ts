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
  estimated_profit?: number;
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
  minimum_viable_rate?: number | null;
}

export interface ApiResponse {
  request: MonteCarloRequest;
  response: MonteCarloResponse;
  simulationData: SimulationDataPoint[];
}

const normalizeProbability = (value: number): number => {
  if (!Number.isFinite(value)) return 0;
  if (value > 1) return value / 100;
  if (value < 0) return 0;
  return value;
};

const toDailyRateForMock = (rate: number): number => {
  if (!Number.isFinite(rate) || rate <= 0) return 0.001;

  // Mock API expects daily rate in [0, 0.01].
  // Frontend usually works with monthly-like values (e.g. 0.15), so convert when needed.
  const daily = rate > 0.01 ? rate / 30 : rate;
  return Math.max(0.0001, Math.min(daily, 0.01));
};

// ── fetch all clients for the banner list ─────────────────────────────
export async function fetchClients(): Promise<Client[]> {
  const res = await fetch(`${API_BASE}/clients`);
  if (!res.ok) throw new Error(`Failed to fetch clients: ${res.status}`);
  const data = await res.json();
  const clients = Array.isArray(data) ? data : data.data ?? [];

  return clients.map((client: any) => ({
    id: String(client.id ?? ""),
    name: client.name ?? `Client ${client.id ?? ""}`,
    occupation: client.occupation ?? "Not provided",
    location: client.location ?? "Not provided",
    incomeFixed: Number(client.incomeFixed ?? 0),
    incomeVariable: Number(client.incomeVariable ?? 0),
    bankConnected: Boolean(client.bankConnected ?? false),
  }));
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

  const mockBody = {
    id: String(body.ssn),
    amount: body.loan_amount,
    min_interest_day: toDailyRateForMock(body.max_interest_rate),
  };

  const res = await fetch(`${API_BASE}/simulate`, {
    method:  "POST",
    headers: { "Content-Type": "application/json" },
    body:    JSON.stringify(mockBody),
  });

  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: res.statusText }));
    throw new Error(err.error ?? err.detail ?? "Simulation failed");
  }

  const raw = await res.json();
  const scenarios = Array.isArray(raw.data) ? raw.data : [];

  const viableScenarios = scenarios.filter((item: any) => (item.expected_profit ?? 0) >= body.min_profit);
  const recommendedScenario = (viableScenarios[0] ?? scenarios[scenarios.length - 1]) ?? {
    interest_rate: body.max_interest_rate,
    risk: 1,
    expected_profit: 0,
  };

  const recommendedProbability = 1 - normalizeProbability(recommendedScenario.risk ?? 0);

  const data = {
    request: body,
    response: {
      recommended_interest_rate: recommendedScenario.interest_rate ?? body.max_interest_rate,
      viable: viableScenarios.length > 0,
      paths: [],
      minimum_viable_rate: viableScenarios[0]?.interest_rate ?? null,
      statistics: {
        avg_profit: recommendedScenario.expected_profit ?? 0,
        estimated_profit: recommendedScenario.expected_profit ?? 0,
        profit_std_dev: 0,
        repayment_probability: recommendedProbability,
        max_profit: scenarios.length ? Math.max(...scenarios.map((item: any) => item.expected_profit ?? 0)) : 0,
        min_profit: scenarios.length ? Math.min(...scenarios.map((item: any) => item.expected_profit ?? 0)) : 0,
        confidence_interval: {
          lower: 0,
          upper: 0,
          confidence: 0,
        },
      },
      interest_rate_sweep: scenarios.map((item: any) => ({
        interest_rate: item.interest_rate ?? 0,
        avg_profit: item.expected_profit ?? 0,
        repayment_probability: 1 - normalizeProbability(item.risk ?? 0),
      })),
    },
  };

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

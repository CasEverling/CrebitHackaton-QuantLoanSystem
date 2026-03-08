// Interfaces para tipagem
interface MonteCarloRequest {
  ssn: number;
  loan_amount: number;
  max_interest_rate: number;
  pay_day: number;
  min_profit: number;
}

interface Statistics {
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

interface InterestRateSweep {
  interest_rate: number;
  avg_profit: number;
  repayment_probability: number;
}

interface MonteCarloResponse {
  recommended_interest_rate: number;
  viable: boolean;
  paths: number[][];
  statistics: Statistics;
  interest_rate_sweep: InterestRateSweep[];
}

interface SimulationDataPoint {
  name: string;
  avg: number;
  min: number;
  max: number;
}

interface ApiResponse {
  request: MonteCarloRequest;
  response: MonteCarloResponse;
  simulationData: SimulationDataPoint[];
}

interface UserData {
  [key: string]: any; // TODO: Definir interface mais específica
}

export const apiService = {
  async simulateCredit(_userData: UserData): Promise<ApiResponse> {
    await new Promise<void>(resolve => setTimeout(resolve, 1000));

    const realBackendData: {
      request: MonteCarloRequest;
      response: MonteCarloResponse;
    } = {
      "request": { "ssn": 123456789, "loan_amount": 10000.00, "max_interest_rate": 0.15, "pay_day": 10, "min_profit": 500.00 },
      "response": {
        "recommended_interest_rate": 0.087,
        "viable": true,
        "paths": [
          [1250.50, 1380.20, 900.10, 1450.00, 1200.75],
          [1100.00, 1420.30, 1350.80, 980.50, 1300.00]
        ],
        "statistics": {
          "avg_profit": 1243.50,
          "profit_std_dev": 312.80,
          "repayment_probability": 0.873,
          "max_profit": 3200.00,
          "min_profit": -800.00,
          "confidence_interval": { "lower": 620.00, "upper": 1980.00, "confidence": 0.95 }
        },
        "interest_rate_sweep": [
          { "interest_rate": 0.05, "avg_profit": 420.00, "repayment_probability": 0.95 },
          { "interest_rate": 0.08, "avg_profit": 780.00, "repayment_probability": 0.89 },
          { "interest_rate": 0.12, "avg_profit": 1100.00, "repayment_probability": 0.81 },
          { "interest_rate": 0.15, "avg_profit": 1320.00, "repayment_probability": 0.74 }
        ]
      }
    };

    // Formata os "paths" para o gráfico de linhas (opcional, mas legal para o visual)
    const formattedPaths: SimulationDataPoint[] = realBackendData.response.paths[0].map((val: number, i: number) => ({
      name: `T${i+1}`,
      avg: val,
      min: realBackendData.response.statistics.confidence_interval.lower,
      max: realBackendData.response.statistics.confidence_interval.upper
    }));

    return {
      ...realBackendData,
      simulationData: formattedPaths // Injeta os dados formatados para o gráfico
    };
  }
};
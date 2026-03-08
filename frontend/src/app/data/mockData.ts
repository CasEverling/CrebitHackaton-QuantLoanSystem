// Mock data para simulação de clientes e resultados de Monte Carlo

export interface Client {
  id: string;
  name: string;
  occupation: string;
  location: string;
  riskScore: number;
  avatar: string;
  incomeFixed: number;
  incomeVariable: number;
  bankConnected: boolean;
}

export interface MonteCarloRequest {
  ss: number;
  loan_amount: number;
  pay_day: string;
}

export interface MonteCarloStatistics {
  avg_profit: number;
  repayment_probability: number;
  profit_std_dev: number;
  confidence_interval: {
    lower: number;
    upper: number;
  };
}

export interface InterestRateSweep {
  interest_rate: number;
  avg_profit: number;
  repayment_probability: number;
}

export interface MonteCarloResponse {
  request: MonteCarloRequest;
  statistics: MonteCarloStatistics;
  interest_rate_sweep: InterestRateSweep[];
  recommended_rate: number;
}

export interface SimulationDataPoint {
  name: string;
  min: number;
  max: number;
  avg: number;
}

export const mockClients: Client[] = [
  {
    id: "1",
    name: "Carlos Silva",
    occupation: "Motorista Uber",
    location: "São Paulo, SP",
    riskScore: 78,
    avatar: "CS",
    incomeFixed: 0,
    incomeVariable: 100,
    bankConnected: true
  },
  {
    id: "2",
    name: "Ana Santos",
    occupation: "Entregadora iFood",
    location: "Rio de Janeiro, RJ",
    riskScore: 85,
    avatar: "AS",
    incomeFixed: 20,
    incomeVariable: 80,
    bankConnected: true
  },
  {
    id: "3",
    name: "João Oliveira",
    occupation: "Designer Freelance",
    location: "Belo Horizonte, MG",
    riskScore: 72,
    avatar: "JO",
    incomeFixed: 40,
    incomeVariable: 60,
    bankConnected: false
  },
  {
    id: "4",
    name: "Maria Costa",
    occupation: "Desenvolvedora Freelance",
    location: "Curitiba, PR",
    riskScore: 92,
    avatar: "MC",
    incomeFixed: 30,
    incomeVariable: 70,
    bankConnected: true
  },
  {
    id: "5",
    name: "Pedro Ferreira",
    occupation: "Motorista 99",
    location: "Brasília, DF",
    riskScore: 65,
    avatar: "PF",
    incomeFixed: 10,
    incomeVariable: 90,
    bankConnected: true
  },
  {
    id: "6",
    name: "Julia Almeida",
    occupation: "Fotógrafa Freelance",
    location: "Porto Alegre, RS",
    riskScore: 88,
    avatar: "JA",
    incomeFixed: 25,
    incomeVariable: 75,
    bankConnected: false
  }
];

export const generateMonteCarloData = (clientId: string, loanAmount: number, payDay: string): MonteCarloResponse => {
  const client = mockClients.find(c => c.id === clientId);
  const baseRate = client ? (100 - client.riskScore) / 10 : 3;
  
  return {
    request: {
      ss: parseFloat(clientId),
      loan_amount: loanAmount,
      pay_day: payDay
    },
    statistics: {
      avg_profit: loanAmount * 1.15,
      repayment_probability: client ? client.riskScore : 75,
      profit_std_dev: loanAmount * 0.08,
      confidence_interval: {
        lower: loanAmount * 0.95,
        upper: loanAmount * 1.35
      }
    },
    interest_rate_sweep: [
      { interest_rate: baseRate - 1, avg_profit: loanAmount * 1.10, repayment_probability: (client?.riskScore || 75) + 8 },
      { interest_rate: baseRate, avg_profit: loanAmount * 1.15, repayment_probability: client?.riskScore || 75 },
      { interest_rate: baseRate + 1, avg_profit: loanAmount * 1.20, repayment_probability: (client?.riskScore || 75) - 5 },
      { interest_rate: baseRate + 2, avg_profit: loanAmount * 1.25, repayment_probability: (client?.riskScore || 75) - 12 },
      { interest_rate: baseRate + 3, avg_profit: loanAmount * 1.30, repayment_probability: (client?.riskScore || 75) - 18 }
    ],
    recommended_rate: baseRate
  };
};

export const generateSimulationData = (loanAmount: number): SimulationDataPoint[] => {
  return [
    { name: "Sem 1", min: loanAmount * 0.85, max: loanAmount * 1.15, avg: loanAmount * 1.00 },
    { name: "Sem 2", min: loanAmount * 0.90, max: loanAmount * 1.20, avg: loanAmount * 1.05 },
    { name: "Sem 3", min: loanAmount * 0.88, max: loanAmount * 1.25, avg: loanAmount * 1.08 },
    { name: "Sem 4", min: loanAmount * 0.92, max: loanAmount * 1.28, avg: loanAmount * 1.12 },
    { name: "Sem 5", min: loanAmount * 0.95, max: loanAmount * 1.30, avg: loanAmount * 1.15 },
    { name: "Sem 6", min: loanAmount * 0.93, max: loanAmount * 1.32, avg: loanAmount * 1.18 },
    { name: "Sem 7", min: loanAmount * 0.96, max: loanAmount * 1.33, avg: loanAmount * 1.20 },
    { name: "Sem 8", min: loanAmount * 0.94, max: loanAmount * 1.35, avg: loanAmount * 1.22 }
  ];
};

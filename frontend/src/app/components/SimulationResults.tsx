import React from 'react';
import InterestCard from './results/InterestCard';
import { SensitivityAnalysis } from './results/SensitivityAnalysis';
import MonteCarloChart from './results/MonteCarloChart';
import ExplainableAI from './results/ExplainableAI';
import { StatsGrid } from './results/StatsGrid';
import { RiskCard } from './results/RiskCard';
import { Client } from '../services/apiService';

interface SimulationResultsProps {
  results: any;
  client?: Client | null;
}

export default function SimulationResults({ results, client }: SimulationResultsProps): React.JSX.Element {
  if (!results) return <></>;

  const { response } = results;
  const { statistics, interest_rate_sweep, recommended_interest_rate, request } = response;

  return (
    <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
      {/* Left column */}
      <div className="lg:col-span-1 space-y-6">
        <InterestCard
          rate={recommended_interest_rate}
          probability={1 - (statistics?.repayment_probability || 0)}
        />

        <StatsGrid
          avgProfit={statistics?.avg_profit || 0}
          stdDev={statistics?.profit_std_dev || 0}
          confidenceLower={statistics?.confidence_interval?.lower || 0}
          confidenceUpper={statistics?.confidence_interval?.upper || 0}
        />

        <RiskCard
          riskScore={Math.round((1 - (statistics?.repayment_probability || 0)) * 100)}
          occupation={client?.occupation ?? "—"}
          location={client?.location ?? "—"}
        />

        <ExplainableAI
          statistics={statistics}
          payDay={request?.pay_day}
        />
      </div>

      {/* Right column */}
      <div className="lg:col-span-2 space-y-6">
        <MonteCarloChart data={results.simulationData} />
        <SensitivityAnalysis
          sweepData={interest_rate_sweep}
          recommendedRate={recommended_interest_rate}
        />
      </div>
    </div>
  );
}

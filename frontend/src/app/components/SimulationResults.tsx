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
  const { statistics, interest_rate_sweep, recommended_interest_rate, viable, minimum_viable_rate } = response;
  const { request } = results;

  return (
    <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
      {/* Left column */}
      <div className="lg:col-span-1 space-y-6">
        <InterestCard
          rate={recommended_interest_rate}
          probability={1 - (statistics?.repayment_probability || 0)}
        />

        <StatsGrid
          estimatedProfit={statistics?.estimated_profit ?? statistics?.avg_profit ?? 0}
          repaymentProbability={statistics?.repayment_probability || 0}
          minimumViableRate={minimum_viable_rate ?? null}
          viable={Boolean(viable)}
        />

        <RiskCard
          riskScore={Math.round((1 - (statistics?.repayment_probability || 0)) * 100)}
          occupation={client?.occupation ?? "—"}
          location={client?.location ?? "—"}
        />

        <ExplainableAI
          statistics={statistics}
          viable={Boolean(viable)}
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

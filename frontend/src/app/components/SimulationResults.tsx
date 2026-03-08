import React from 'react';
import InterestCard from './results/InterestCard';
import SensitivitySweep from './results/SensitivitySweep';
import MonteCarloChart from './results/MonteCarloChart';
import ExplainableAI from './results/ExplainableAI';

interface SimulationResultsProps {
  results: any; // TODO: Definir interface mais específica
}

interface StatCardProps {
  label: string;
  value: number | undefined;
  color: string;
  isMono?: boolean;
}

export default function SimulationResults({ results }: SimulationResultsProps): React.JSX.Element {
    if (!results) return <></>;

    const { response } = results;
    const { statistics, interest_rate_sweep, recommended_interest_rate, request } = response;

	return (
		<div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
			{/* Coluna da Esquerda: Cards e IA */}
			<div className="lg:col-span-1 space-y-6">
				<InterestCard
					rate={recommended_interest_rate}
					probability={statistics?.repayment_probability}
				/>

				<div className="grid grid-cols-2 gap-4">
                	<StatCard label="Desvio Padrão" value={statistics?.profit_std_dev} color="text-slate-700" />
                	<StatCard label="Lucro Máximo" value={statistics?.max_profit} color="text-green-600" isMono />
            	</div>

				<ExplainableAI
					statistics={statistics}
					payDay={request?.pay_day}
				/>
			</div>

			{/* Coluna da Direita: Gráficos e Sweep */}
			<div className="lg:col-span-2 space-y-6">
				<MonteCarloChart data={results.simulationData} />
				<SensitivitySweep data={interest_rate_sweep} />
			</div>
		</div>
	);
}

const StatCard: React.FC<StatCardProps> = ({ label, value, color, isMono }) => (
    <div className="bg-white p-4 rounded-2xl border border-slate-200 text-center">
        <p className="text-slate-400 text-[10px] uppercase font-bold">{label}</p>
        <p className={`text-lg font-bold ${color} ${isMono ? 'font-mono' : ''}`}>
            R$ {value || 0}
        </p>
    </div>
);
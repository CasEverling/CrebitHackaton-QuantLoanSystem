import React from 'react';

export default function SensitivitySweep({ data }) {
    if (!data) return null;
    return (
        <div className="bg-white p-6 rounded-2xl border border-slate-200">
            <h3 className="font-bold text-slate-800 mb-4 italic">Análise de Sensibilidade</h3>
            <div className="space-y-3">
                {data.map((item, idx) => (
                    <div key={idx} className="flex items-center justify-between p-3 bg-slate-50 rounded-lg border border-slate-100">
                        <div>
                            <p className="text-sm font-bold text-indigo-600">
                                {(item.interest_rate * 100).toFixed(0)}% Juros
                            </p>
                            <p className="text-[10px] text-slate-400 uppercase">
                                Lucro Médio: R$ {item.avg_profit}
                            </p>
                        </div>
                        <div className="text-right">
                            <p className="text-xs font-bold text-slate-700">
                                {(item.repayment_probability * 100).toFixed(0)}% Solvência
                            </p>
                            <div className="w-20 h-1.5 bg-slate-200 rounded-full mt-1">
                                <div
                                    className="h-full bg-green-500 rounded-full"
                                    style={{ width: `${item.repayment_probability * 100}%` }}
                                />
                            </div>
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
}
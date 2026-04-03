import React from 'react';
import { ShieldCheck } from 'lucide-react';

export default function ExplainableAI({ statistics, payDay, viable }) {
    const repaymentProbability = Math.max(0, Math.min(1, statistics?.repayment_probability || 0));
    const estimatedProfit = statistics?.estimated_profit ?? statistics?.avg_profit ?? 0;

    return (
        <div className="bg-slate-900 text-slate-300 p-6 rounded-2xl border border-slate-800">
            <h4 className="text-white font-bold mb-2 flex items-center gap-2 text-sm">
                <ShieldCheck size={16} className="text-indigo-400" /> 
                Algorithm Rationale (C++ Engine)
            </h4>
            <p className="text-xs leading-relaxed opacity-80 italic">
                Offer generated with a repayment probability of {(repaymentProbability * 100).toFixed(1)}%.
                Estimated profit is <span className="text-indigo-300">$ {Number(estimatedProfit).toFixed(2)}</span>
                for a {payDay || 0} day repayment target.
            </p>
            <div className="mt-4 pt-4 border-t border-slate-800 flex justify-between items-center">
                <span className="text-[10px] uppercase tracking-widest font-bold">Model Status</span>
                <span className={`${viable ? "text-green-400" : "text-red-400"} text-[10px] font-bold flex items-center gap-1`}>
                    <div className={`w-1.5 h-1.5 ${viable ? "bg-green-400" : "bg-red-400"} rounded-full animate-pulse`} />
                    {viable ? "VIABLE" : "NOT VIABLE"}
                </span>
            </div>
        </div>
    );
}
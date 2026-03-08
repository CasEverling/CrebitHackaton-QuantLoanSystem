import React from 'react';
import { ShieldCheck } from 'lucide-react';

export default function ExplainableAI({ statistics, payDay }) {
    return (
        <div className="bg-slate-900 text-slate-300 p-6 rounded-2xl border border-slate-800">
            <h4 className="text-white font-bold mb-2 flex items-center gap-2 text-sm">
                <ShieldCheck size={16} className="text-indigo-400" /> 
                Racional do Algoritmo (C++ Engine)
            </h4>
            <p className="text-xs leading-relaxed opacity-80 italic">
                Oferta gerada processando {(statistics?.paths_profitable * 100 || 0).toFixed(1)}% de caminhos lucrativos. 
                O desvio padrão de <span className="text-indigo-300">R$ {statistics?.profit_std_dev}</span> indica volatilidade estável para o período de {payDay || 0} dias.
            </p>
            <div className="mt-4 pt-4 border-t border-slate-800 flex justify-between items-center">
                <span className="text-[10px] uppercase tracking-widest font-bold">Status do Modelo</span>
                <span className="text-green-400 text-[10px] font-bold flex items-center gap-1">
                    <div className="w-1.5 h-1.5 bg-green-400 rounded-full animate-pulse" />
                    VIÁVEL ({(statistics?.confidence_interval?.confidence * 100 || 0)}% CONFIANÇA)
                </span>
            </div>
        </div>
    );
}
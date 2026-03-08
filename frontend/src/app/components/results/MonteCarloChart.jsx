import React from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

export default function MonteCarloChart({ data }) {
    // Se não tiver data, mostra aviso
    if (!data || data.length === 0) {
        return (
            <div className="bg-white p-6 rounded-2xl border border-slate-200 w-full">
                <h3 className="font-bold text-slate-800 mb-4 text-sm uppercase tracking-wide">Projeção de Lucro vs Risco</h3>
                <div className="h-80 w-full flex items-center justify-center bg-slate-50 rounded-lg">
                    <p className="text-slate-500 text-sm">⚠️ Nenhum dado disponível para o gráfico</p>
                </div>
            </div>
        );
    }

    return (
        <div className="bg-white p-6 rounded-2xl border border-slate-200 w-full">
            <h3 className="font-bold text-slate-800 mb-4 text-sm uppercase tracking-wide">Projeção de Lucro vs Risco</h3>
            <div style={{ width: '100%', height: '300px' }}> 
                <ResponsiveContainer width="100%" height="100%" minWidth={0}>
                    <AreaChart data={data} margin={{ top: 10, right: 30, left: 0, bottom: 0 }}>
                        <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" />
                        <XAxis dataKey="name" axisLine={false} tickLine={false} tick={{fontSize: 10}} />
                        <YAxis axisLine={false} tickLine={false} tickFormatter={(v) => `R$${v}`} tick={{fontSize: 10}} />
                        <Tooltip formatter={(value) => `R$ ${value.toFixed(2)}`} />
                        <Area type="monotone" dataKey="max" stroke="none" fill="#e0e7ff" fillOpacity={0.5} name="Limite Superior" />
                        <Area type="monotone" dataKey="min" stroke="none" fill="#f8fafc" fillOpacity={1} name="Limite Inferior" />
                        <Area type="monotone" dataKey="avg" stroke="#6366f1" strokeWidth={3} fill="none" name="Projeção" />
                    </AreaChart>
                </ResponsiveContainer>
            </div>
        </div>
    );
}
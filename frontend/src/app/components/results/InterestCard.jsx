import React from 'react';

export default function InterestCard({ rate, probability }) {
    return (
        <div className="bg-indigo-700 text-white p-6 rounded-3xl shadow-xl">
            <p className="text-indigo-200 text-xs font-bold uppercase">Taxa de Juros Recomendada</p>
            <h2 className="text-5xl font-black mt-1">
                {(rate * 100 || 0).toFixed(1)}%
            </h2>
            <div className="mt-4 flex items-center gap-2 bg-indigo-800/50 p-3 rounded-xl">
                <div className="flex-1 text-xs">
                    <span className="block font-bold">Probabilidade de Quitação:</span>
                    <span className="text-indigo-200">
                        {(probability * 100 || 0).toFixed(1)}%
                    </span>
                </div>
            </div>
        </div>
    );
}
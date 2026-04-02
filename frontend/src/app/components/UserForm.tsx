import React, { useState } from 'react';
import { DollarSign, TrendingUp, Calculator, Calendar } from 'lucide-react';

export default function UserForm({ onSubmit, isLoading }: { onSubmit: (data: any) => void, isLoading: boolean }) {
  const [formData, setFormData] = useState({
    loanAmount:       5000,
    maxInterestRate:  0.15,   // 0–1, displayed as %
    payDay:           10,     // day of month
    minProfit:        500,
  });

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit(formData);
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: Number(value) }));
  };

  return (
    <form onSubmit={handleSubmit} className="space-y-5">

      {/* Loan Amount */}
      <div className="space-y-1.5">
        <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
          <DollarSign size={12} /> Loan Amount ($)
        </label>
        <input
          type="number"
          name="loanAmount"
          value={formData.loanAmount}
          onChange={handleChange}
          min={100}
          step={100}
          className="w-full px-4 py-2.5 bg-input-background border border-border rounded-xl text-sm focus:ring-2 focus:ring-primary/20 outline-none transition-all"
          required
        />
      </div>

      {/* Max Interest Rate slider */}
      <div className="space-y-2 p-4 bg-slate-50 rounded-2xl border border-slate-100">
        <div className="flex justify-between items-center">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground">
            Max Interest Rate
          </label>
          <span className="text-sm font-black text-primary bg-white px-2 py-0.5 rounded shadow-sm border border-border">
            {(formData.maxInterestRate * 100).toFixed(1)}%
          </span>
        </div>
        <input
          type="range"
          name="maxInterestRate"
          min={0.01}
          max={0.50}
          step={0.01}
          value={formData.maxInterestRate}
          onChange={handleChange}
          className="w-full h-1.5 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-primary"
        />
        <div className="flex justify-between text-[10px] text-slate-400">
          <span>1%</span><span>50%</span>
        </div>
      </div>

      {/* Pay Day */}
      <div className="space-y-1.5">
        <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
          <Calendar size={12} /> Repayment Day (day of month)
        </label>
        <input
          type="number"
          name="payDay"
          value={formData.payDay}
          onChange={handleChange}
          min={1}
          max={28}
          className="w-full px-4 py-2.5 bg-input-background border border-border rounded-xl text-sm focus:ring-2 focus:ring-primary/20 outline-none transition-all"
          required
        />
        <p className="text-[10px] text-slate-400">Day of month the borrower repays (1–28)</p>
      </div>

      {/* Min Profit */}
      <div className="space-y-1.5">
        <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
          <Calculator size={12} /> Minimum Acceptable Profit ($)
        </label>
        <input
          type="number"
          name="minProfit"
          value={formData.minProfit}
          onChange={handleChange}
          min={0}
          step={50}
          className="w-full px-4 py-2.5 bg-input-background border border-border rounded-xl text-sm focus:ring-2 focus:ring-primary/20 outline-none transition-all"
          required
        />
      </div>

      {/* Submit */}
      <button
        type="submit"
        disabled={isLoading}
        className="w-full bg-primary text-primary-foreground font-black py-4 rounded-2xl hover:opacity-90 disabled:bg-muted transition-all active:scale-[0.98] shadow-lg shadow-primary/20 flex items-center justify-center gap-3 text-sm uppercase tracking-widest"
      >
        {isLoading ? (
          <div className="animate-spin rounded-full h-5 w-5 border-b-2 border-white" />
        ) : (
          <>
            <TrendingUp size={18} />
            Run Simulation
          </>
        )}
      </button>

      <p className="text-[10px] text-center text-muted-foreground leading-relaxed px-4">
        Parameters feed directly into the <b>Monte Carlo</b> engine running 100,000 paths in C++.
      </p>
    </form>
  );
}

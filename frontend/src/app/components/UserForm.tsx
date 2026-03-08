import React, { useState } from 'react';
import { User, DollarSign, Briefcase, TrendingUp, MapPin, Calculator } from 'lucide-react';

export default function UserForm({ onSubmit, isLoading }: { onSubmit: (data: any) => void, isLoading: boolean }) {
  const [formData, setFormData] = useState({
    name: '',
    profession: 'uber-driver',
    monthlyIncome: 3000,
    monthlyExpenses: 1500,
    riskTolerance: 5,
    location: 'tampa-fl'
  });

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit(formData);
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement | HTMLSelectElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: e.target.type === 'number' || e.target.type === 'range' ? Number(value) : value
    }));
  };

  return (
    <form onSubmit={handleSubmit} className="space-y-5">
      {/* Campo de Nome - Mais compacto */}
      <div className="space-y-1.5">
        <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
          <User size={12} /> Nome do Cliente
        </label>
        <input
          type="text"
          name="name"
          value={formData.name}
          onChange={handleChange}
          className="w-full px-4 py-2.5 bg-input-background border border-border rounded-xl text-sm focus:ring-2 focus:ring-primary/20 outline-none transition-all"
          placeholder="Ex: João Silva"
          required
        />
      </div>

      <div className="grid grid-cols-2 gap-3">
        {/* Profissão */}
        <div className="space-y-1.5">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
            <Briefcase size={12} /> Ocupação
          </label>
          <select
            name="profession"
            value={formData.profession}
            onChange={handleChange}
            className="w-full px-3 py-2.5 bg-input-background border border-border rounded-xl text-xs font-medium outline-none"
          >
            <option value="uber-driver">Uber</option>
            <option value="ifood-delivery">iFood</option>
            <option value="freelancer">Freelancer</option>
          </select>
        </div>

        {/* Localização */}
        <div className="space-y-1.5">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
            <MapPin size={12} /> Região
          </label>
          <select
            name="location"
            value={formData.location}
            onChange={handleChange}
            className="w-full px-3 py-2.5 bg-input-background border border-border rounded-xl text-xs font-medium outline-none"
          >
            <option value="tampa-fl">Tampa, FL</option>
            <option value="sao-paulo-br">SP, BR</option>
          </select>
        </div>
      </div>

      {/* Renda e Despesa na mesma linha */}
      <div className="grid grid-cols-2 gap-3">
        <div className="space-y-1.5">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
            <DollarSign size={12} /> Renda (R$)
          </label>
          <input
            type="number"
            name="monthlyIncome"
            value={formData.monthlyIncome}
            onChange={handleChange}
            className="w-full px-3 py-2.5 bg-input-background border border-border rounded-xl text-sm outline-none"
            required
          />
        </div>
        <div className="space-y-1.5">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground flex items-center gap-2">
            <Calculator size={12} /> Gastos (R$)
          </label>
          <input
            type="number"
            name="monthlyExpenses"
            value={formData.monthlyExpenses}
            onChange={handleChange}
            className="w-full px-3 py-2.5 bg-input-background border border-border rounded-xl text-sm outline-none"
            required
          />
        </div>
      </div>

      {/* Risco Slider - Estilo Profissional */}
      <div className="space-y-2 p-4 bg-slate-50 rounded-2xl border border-slate-100">
        <div className="flex justify-between items-center">
          <label className="text-[10px] font-black uppercase tracking-widest text-muted-foreground">
            Apetite de Risco
          </label>
          <span className="text-sm font-black text-primary bg-white px-2 py-0.5 rounded shadow-sm border border-border">
            {formData.riskTolerance}/10
          </span>
        </div>
        <input
          type="range"
          name="riskTolerance"
          min="1" max="10"
          value={formData.riskTolerance}
          onChange={handleChange}
          className="w-full h-1.5 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-primary slider"
        />
      </div>

      {/* Botão de Ação Principal */}
      <button
        type="submit"
        disabled={isLoading}
        className="w-full bg-primary text-primary-foreground font-black py-4 rounded-2xl hover:opacity-90 disabled:bg-muted transition-all active:scale-[0.98] shadow-lg shadow-primary/20 flex items-center justify-center gap-3 text-sm uppercase tracking-widest"
      >
        {isLoading ? (
          <div className="animate-spin rounded-full h-5 w-5 border-b-2 border-white"></div>
        ) : (
          <>
            <TrendingUp size={18} />
            Rodar Simulação
          </>
        )}
      </button>

      {/* Nota técnica de rodapé */}
      <p className="text-[10px] text-center text-muted-foreground leading-relaxed px-4">
        Ajustes nestes parâmetros recalcularão a <b>Solvência Preditiva</b> via Monte Carlo em tempo real.
      </p>
    </form>
  );
}
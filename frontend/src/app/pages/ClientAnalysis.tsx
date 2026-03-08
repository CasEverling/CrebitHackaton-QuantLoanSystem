import { useState, useEffect } from "react";
import { useParams, Link } from "react-router-dom";
import { ArrowLeft, Briefcase, MapPin, CheckCircle2, Loader2, Cpu } from "lucide-react";
import { apiService } from "../services/apiService.js";
import SimulationResults from "../components/SimulationResults";
import UserForm from "../components/UserForm";

export default function ClientAnalysis() {
  const { clientId } = useParams(); // Pega o ID da URL
  const [simulationResults, setSimulationResults] = useState<any>(null);
  const [isLoading, setIsLoading] = useState(false);

  // 1. Simulação Inicial (Roda assim que a página abre)
  useEffect(() => {
    if (clientId) {
      // Aqui você pode disparar uma simulação padrão para o ID selecionado
      handleFormSubmit({ clientId, loanAmount: 5000, payDay: "2026-04-08" });
    }
  }, [clientId]);

  const handleFormSubmit = async (formData: any) => {
    setIsLoading(true);
    try {
      const results = await apiService.simulateCredit(formData);
      setSimulationResults(results);
    } catch (error) {
      console.error("Erro na simulação:", error);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-bank-bg animate-in fade-in duration-500">
      
      {/* 1. HEADER ESTILO FIGMA */}
      <div className="bg-white border-b border-border shadow-sm mb-8">
        <div className="max-w-7xl mx-auto px-8 py-6">
          <Link to="/" className="inline-flex items-center gap-2 text-primary hover:underline mb-4 text-sm font-bold">
            <ArrowLeft size={16} /> Voltar para Carteira
          </Link>
          
          <div className="flex justify-between items-start">
            <div className="flex items-center gap-4">
              <div className="w-16 h-16 bg-gradient-to-br from-primary to-indigo-800 rounded-2xl flex items-center justify-center text-white font-black text-2xl shadow-lg">
                {clientId?.charAt(0).toUpperCase() || "U"}
              </div>
              <div>
                <h1 className="text-3xl font-black text-foreground">Análise de Perfil: {clientId}</h1>
                <div className="flex items-center gap-4 mt-1 text-sm text-muted-foreground font-medium">
                  <span className="flex items-center gap-1"><Briefcase size={14}/> Gig Economy Driver</span>
                  <span className="flex items-center gap-1"><MapPin size={14}/> Tampa, FL</span>
                </div>
              </div>
            </div>
            
            <div className="flex items-center gap-2 bg-green-50 text-green-700 px-4 py-2 rounded-xl border border-green-100">
              <CheckCircle2 size={16} />
              <span className="text-xs font-bold uppercase tracking-wider">Open Finance Ativo</span>
            </div>
          </div>
        </div>
      </div>

      {/* 2. CONTEÚDO PRINCIPAL (GRID) */}
      <div className="max-w-7xl mx-auto px-8">
        <div className="grid grid-cols-12 gap-8">
          
          {/* COLUNA ESQUERDA: Controles do Gerente (UserForm) */}
          <div className="col-span-4 space-y-6">
            <div className="bg-card p-6 rounded-[32px] border border-border shadow-sm">
              <h3 className="text-lg font-bold mb-4 flex items-center gap-2">
                <Cpu size={18} className="text-primary" /> Parâmetros de Risco
              </h3>
              {/* O seu UserForm antigo agora atua como o painel de controle lateral */}
              <UserForm onSubmit={handleFormSubmit} isLoading={isLoading} />
            </div>

            {/* CARD DE DICA IA (Inspirado no Figma) */}
            <div className="bg-primary text-primary-foreground p-6 rounded-[32px] shadow-xl">
               <p className="text-xs font-bold opacity-70 uppercase mb-2">Insight da IA</p>
               <p className="text-sm leading-relaxed">
                 O motor de Monte Carlo detectou alta estabilidade nas noites de sexta-feira nesta região. 
                 Sugerimos uma taxa competitiva para garantir a retenção deste cliente.
               </p>
            </div>
          </div>

          {/* COLUNA DIREITA: Resultados e Gráfico */}
          <div className="col-span-8">
            {isLoading ? (
              <div className="h-[500px] flex flex-col items-center justify-center bg-white rounded-[32px] border border-dashed border-slate-300">
                <Loader2 className="animate-spin text-primary mb-4" size={48} />
                <p className="text-muted-foreground font-medium italic">Executando 10.000 caminhos em C++...</p>
              </div>
            ) : simulationResults ? (
              <SimulationResults results={simulationResults} />
            ) : (
              <div className="h-[500px] flex items-center justify-center text-muted-foreground border-2 border-dashed rounded-[32px]">
                Aguardando parâmetros para simulação...
              </div>
            )}
          </div>

        </div>
      </div>
    </div>
  );
}
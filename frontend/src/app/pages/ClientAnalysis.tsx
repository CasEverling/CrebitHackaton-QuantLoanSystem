import { useState, useEffect } from "react";
import { useParams, Link } from "react-router-dom";
import { ArrowLeft, Briefcase, MapPin, CheckCircle2, Loader2, Cpu } from "lucide-react";
import { apiService, fetchClients, Client } from "../services/apiService.js";
import SimulationResults from "../components/SimulationResults";
import UserForm from "../components/UserForm";

export default function ClientAnalysis() {
  const { clientId } = useParams();
  const [simulationResults, setSimulationResults] = useState<any>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [client, setClient] = useState<Client | null>(null);

  // Load client info for the header
  useEffect(() => {
    if (clientId) {
      fetchClients()
        .then(clients => setClient(clients.find(c => c.id === clientId) ?? null))
        .catch(err => console.error("Failed to load client:", err));
    }
  }, [clientId]);

  // Run default simulation on page open
  useEffect(() => {
    if (clientId) {
      handleFormSubmit({
        clientId,
        loanAmount:      5000,
        maxInterestRate: 0.15,
        payDay:          10,
        minProfit:       500,
      });
    }
  }, [clientId]);

  const handleFormSubmit = async (formData: any) => {
    setIsLoading(true);
    try {
      const results = await apiService.simulateCredit({ ...formData, clientId });
      setSimulationResults(results);
    } catch (error) {
      console.error("Simulation error:", error);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-bank-bg animate-in fade-in duration-500">

      {/* HEADER */}
      <div className="bg-white border-b border-border shadow-sm mb-8">
        <div className="max-w-7xl mx-auto px-8 py-6">
          <Link to="/" className="inline-flex items-center gap-2 text-primary hover:underline mb-4 text-sm font-bold">
            <ArrowLeft size={16} /> Back to Portfolio
          </Link>

          <div className="flex justify-between items-start">
            <div className="flex items-center gap-4">
              <div className="w-16 h-16 bg-gradient-to-br from-primary to-indigo-800 rounded-2xl flex items-center justify-center text-white font-black text-2xl shadow-lg">
                {clientId?.charAt(0).toUpperCase() || "U"}
              </div>
              <div>
                <h1 className="text-3xl font-black text-foreground">
                  {client?.name ?? `Client ${clientId}`}
                </h1>
                <div className="flex items-center gap-4 mt-1 text-sm text-muted-foreground font-medium">
                  <span className="flex items-center gap-1">
                    <Briefcase size={14}/> {client?.occupation ?? "—"}
                  </span>
                  <span className="flex items-center gap-1">
                    <MapPin size={14}/> {client?.location ?? "—"}
                  </span>
                </div>
              </div>
            </div>

            <div className={`flex items-center gap-2 px-4 py-2 rounded-xl border ${
              client?.bankConnected
                ? "bg-green-50 text-green-700 border-green-100"
                : "bg-yellow-50 text-yellow-700 border-yellow-100"
            }`}>
              <CheckCircle2 size={16} />
              <span className="text-xs font-bold uppercase tracking-wider">
                {client?.bankConnected ? "Open Finance Active" : "Bank Not Connected"}
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* MAIN CONTENT */}
      <div className="max-w-7xl mx-auto px-8">
        <div className="grid grid-cols-12 gap-8">

          {/* LEFT: controls */}
          <div className="col-span-4 space-y-6">
            <div className="bg-card p-6 rounded-[32px] border border-border shadow-sm">
              <h3 className="text-lg font-bold mb-4 flex items-center gap-2">
                <Cpu size={18} className="text-primary" /> Risk Parameters
              </h3>
              <UserForm onSubmit={handleFormSubmit} isLoading={isLoading} />
            </div>

            <div className="bg-primary text-primary-foreground p-6 rounded-[32px] shadow-xl">
              <p className="text-xs font-bold opacity-70 uppercase mb-2">AI Insight</p>
              <p className="text-sm leading-relaxed">
                The Monte Carlo engine detected high stability on Friday nights in this region.
                We suggest a competitive rate to ensure customer retention.
              </p>
            </div>
          </div>

          {/* RIGHT: results */}
          <div className="col-span-8">
            {isLoading ? (
              <div className="h-[500px] flex flex-col items-center justify-center bg-white rounded-[32px] border border-dashed border-slate-300">
                <Loader2 className="animate-spin text-primary mb-4" size={48} />
                <p className="text-muted-foreground font-medium italic">Running Monte Carlo simulation in C++...</p>
              </div>
            ) : simulationResults ? (
              <SimulationResults results={simulationResults} client={client} />
            ) : (
              <div className="h-[500px] flex items-center justify-center text-muted-foreground border-2 border-dashed rounded-[32px]">
                Waiting for simulation parameters...
              </div>
            )}
          </div>

        </div>
      </div>
    </div>
  );
}

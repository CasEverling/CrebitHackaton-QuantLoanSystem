import { useState } from "react";
import { Link } from "react-router-dom";
import { mockClients, Client } from "../data/mockData";
import { Search, UserPlus, CheckCircle, AlertCircle, XCircle } from "lucide-react";

export function ClientList() {
  const [searchTerm, setSearchTerm] = useState("");
  const [clients] = useState<Client[]>(mockClients);

  const filteredClients = clients.filter(
    (client) =>
      client.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
      client.occupation.toLowerCase().includes(searchTerm.toLowerCase()) ||
      client.location.toLowerCase().includes(searchTerm.toLowerCase())
  );

  const getRiskBadge = (score: number) => {
    if (score >= 80) {
      return { icon: CheckCircle, color: "text-green-600 bg-green-100", label: "Baixo" };
    } else if (score >= 60) {
      return { icon: AlertCircle, color: "text-yellow-600 bg-yellow-100", label: "Moderado" };
    } else {
      return { icon: XCircle, color: "text-red-600 bg-red-100", label: "Alto" };
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-50 to-slate-100">
      {/* Header */}
      <div className="bg-white border-b border-slate-200 shadow-sm">
        <div className="max-w-7xl mx-auto px-8 py-6">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-slate-900">Carteira de Clientes</h1>
              <p className="text-slate-600 mt-1">
                Gerencie e analise crédito para trabalhadores da Gig Economy
              </p>
            </div>
            <button className="flex items-center gap-2 bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-xl font-semibold transition-colors shadow-sm">
              <UserPlus className="w-5 h-5" />
              Adicionar Cliente
            </button>
          </div>
        </div>
      </div>

      {/* Content */}
      <div className="max-w-7xl mx-auto px-8 py-8">
        {/* Search Bar */}
        <div className="bg-white rounded-2xl shadow-sm border border-slate-200 p-6 mb-6">
          <div className="relative">
            <Search className="absolute left-4 top-1/2 transform -translate-y-1/2 w-5 h-5 text-slate-400" />
            <input
              type="text"
              placeholder="Buscar por nome, ocupação ou localização..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="w-full pl-12 pr-4 py-3 border border-slate-300 rounded-xl focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-transparent"
            />
          </div>
        </div>

        {/* Stats Cards */}
        <div className="grid grid-cols-4 gap-4 mb-6">
          <div className="bg-white rounded-2xl p-5 shadow-sm border border-slate-200">
            <p className="text-sm text-slate-600 mb-1">Total de Clientes</p>
            <p className="text-3xl font-bold text-slate-900">{clients.length}</p>
          </div>
          <div className="bg-white rounded-2xl p-5 shadow-sm border border-slate-200">
            <p className="text-sm text-slate-600 mb-1">Risco Baixo</p>
            <p className="text-3xl font-bold text-green-600">
              {clients.filter((c) => c.riskScore >= 80).length}
            </p>
          </div>
          <div className="bg-white rounded-2xl p-5 shadow-sm border border-slate-200">
            <p className="text-sm text-slate-600 mb-1">Risco Moderado</p>
            <p className="text-3xl font-bold text-yellow-600">
              {clients.filter((c) => c.riskScore >= 60 && c.riskScore < 80).length}
            </p>
          </div>
          <div className="bg-white rounded-2xl p-5 shadow-sm border border-slate-200">
            <p className="text-sm text-slate-600 mb-1">Risco Alto</p>
            <p className="text-3xl font-bold text-red-600">
              {clients.filter((c) => c.riskScore < 60).length}
            </p>
          </div>
        </div>

        {/* Clients Table */}
        <div className="bg-white rounded-2xl shadow-sm border border-slate-200 overflow-hidden">
          <div className="overflow-x-auto">
            <table className="w-full">
              <thead className="bg-slate-50 border-b border-slate-200">
                <tr>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Cliente
                  </th>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Ocupação
                  </th>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Localização
                  </th>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Score de Risco
                  </th>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Status
                  </th>
                  <th className="px-6 py-4 text-left text-xs font-semibold text-slate-700 uppercase tracking-wider">
                    Ações
                  </th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-200">
                {filteredClients.map((client) => {
                  const riskBadge = getRiskBadge(client.riskScore);
                  const RiskIcon = riskBadge.icon;

                  return (
                    <tr
                      key={client.id}
                      className="hover:bg-slate-50 transition-colors"
                    >
                      <td className="px-6 py-4">
                        <div className="flex items-center gap-3">
                          <div className="w-10 h-10 bg-gradient-to-br from-indigo-500 to-purple-600 rounded-full flex items-center justify-center text-white font-bold text-sm">
                            {client.avatar}
                          </div>
                          <div>
                            <p className="font-semibold text-slate-900">{client.name}</p>
                            <p className="text-xs text-slate-500">ID: {client.id}</p>
                          </div>
                        </div>
                      </td>
                      <td className="px-6 py-4">
                        <p className="text-sm text-slate-900">{client.occupation}</p>
                      </td>
                      <td className="px-6 py-4">
                        <p className="text-sm text-slate-900">{client.location}</p>
                      </td>
                      <td className="px-6 py-4">
                        <div className="flex items-center gap-2">
                          <span className="text-2xl font-bold text-slate-900">
                            {client.riskScore}
                          </span>
                          <span className="text-sm text-slate-500">/100</span>
                        </div>
                      </td>
                      <td className="px-6 py-4">
                        <div className={`inline-flex items-center gap-2 px-3 py-1 rounded-lg ${riskBadge.color}`}>
                          <RiskIcon className="w-4 h-4" />
                          <span className="text-xs font-semibold">{riskBadge.label}</span>
                        </div>
                      </td>
                      <td className="px-6 py-4">
                        <Link
                          to={`/analysis/${client.id}`}
                          className="inline-block px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white text-sm font-semibold rounded-lg transition-colors"
                        >
                          Analisar
                        </Link>
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
}

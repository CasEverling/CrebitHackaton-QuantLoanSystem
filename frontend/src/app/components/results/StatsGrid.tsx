import { TrendingUp, DollarSign, Percent, CheckCircle2 } from "lucide-react";
import { Card, CardContent } from "../../ui/card";

interface StatsGridProps {
  estimatedProfit: number;
  repaymentProbability: number;
  minimumViableRate: number | null;
  viable: boolean;
}

export function StatsGrid({ estimatedProfit, repaymentProbability, minimumViableRate, viable }: StatsGridProps) {
  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: "USD",
      minimumFractionDigits: 0
    }).format(value);
  };

  const formatPercent = (value: number) => `${(value * 100).toFixed(1)}%`;

  const stats = [
    {
      label: "Estimated Profit",
      value: formatCurrency(estimatedProfit),
      icon: DollarSign,
      color: "text-indigo-600"
    },
    {
      label: "Repayment Probability",
      value: formatPercent(repaymentProbability),
      icon: Percent,
      color: "text-purple-600"
    },
    {
      label: "Minimum Viable Rate",
      value: minimumViableRate == null ? "Not Found" : `${(minimumViableRate * 100).toFixed(2)}%`,
      icon: TrendingUp,
      color: "text-blue-600"
    },
    {
      label: "Recommendation Status",
      value: viable ? "Viable" : "Not Viable",
      icon: CheckCircle2,
      color: "text-emerald-600"
    }
  ];

  return (
    <div className="grid grid-cols-2 gap-4">
      {stats.map((stat, index) => {
        const Icon = stat.icon;
        return (
          <Card key={index} className="hover:shadow-md transition-shadow">
            <CardContent className="p-4">
              <div className="flex items-center justify-between mb-3">
                <div className="w-10 h-10 bg-slate-100 rounded-lg flex items-center justify-center">
                  <Icon className={`w-5 h-5 ${stat.color}`} />
                </div>
              </div>
              <div>
                <p className="text-xs text-slate-600 mb-1">{stat.label}</p>
                <p className={`text-lg font-bold ${stat.color}`}>{stat.value}</p>
              </div>
            </CardContent>
          </Card>
        );
      })}
    </div>
  );
}
import { InterestRateSweep } from "../../data/mockData";
import { TrendingUp, TrendingDown } from "lucide-react";
import { Card, CardContent } from "../../ui/card";
import { Progress } from "../../ui/progress";

interface SensitivityAnalysisProps {
  sweepData: InterestRateSweep[];
  recommendedRate: number;
}

export function SensitivityAnalysis({ sweepData, recommendedRate }: SensitivityAnalysisProps) {
  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: "USD",
      minimumFractionDigits: 0
    }).format(value);
  };

  return (
    <Card>
      <CardContent className="p-6">
        <div className="mb-6">
          <h3 className="text-xl font-bold text-slate-900">
            Interest Rate Sensitivity Analysis
          </h3>
          <p className="text-sm text-slate-600 mt-1">
            Comparison of interest rate scenarios vs. repayment probability
          </p>
        </div>

        <div className="space-y-3">
          {sweepData.map((item, index) => {
            const isRecommended = Math.abs(item.interest_rate - recommendedRate) < 0.0005;
            const probability = (1 - item.repayment_probability) * 100; // Convert to default probability percentage

            return (
              <div
                key={index}
                className={`p-4 rounded-xl border-2 transition-all ${
                  isRecommended
                    ? "bg-indigo-50 border-indigo-500"
                    : "bg-slate-50 border-slate-200 hover:border-slate-300"
                }`}
              >
                <div className="flex items-center justify-between mb-3">
                  <div className="flex items-center gap-3">
                    <div className={`px-3 py-1 rounded-lg font-bold text-sm ${
                      isRecommended
                        ? "bg-indigo-600 text-white"
                        : "bg-slate-200 text-slate-700"
                    }`}>
                      {(item.interest_rate * 100).toFixed(1)}% p.m.
                    </div>
                    {isRecommended && (
                      <span className="text-xs font-semibold text-indigo-600 bg-indigo-100 px-2 py-1 rounded">
                        RECOMMENDED
                      </span>
                    )}
                  </div>
                  <div className="text-right">
                    <p className="text-xs text-slate-600">Average Profit</p>
                    <p className="font-bold text-slate-900">{formatCurrency(item.avg_profit)}</p>
                  </div>
                </div>

                <div className="space-y-2">
                  <div className="flex items-center justify-between text-sm">
                    <span className="text-slate-600">Default Probability</span>
                    <span className="font-bold text-slate-900">{probability.toFixed(1)}%</span>
                  </div>

                  {/* Barra de progresso usando componente UI */}
                  <Progress
                    value={probability}
                    className={`h-3 ${
                      probability <= 20
                        ? "[&>div]:bg-green-500"
                        : probability <= 40
                        ? "[&>div]:bg-yellow-500"
                        : "[&>div]:bg-red-500"
                    }`}
                  />

                  {/* Risk Indicator */}
                  <div className="flex items-center gap-2 text-xs">
                    {probability <= 20 ? (
                      <>
                        <TrendingUp className="w-4 h-4 text-green-600" />
                        <span className="text-green-700 font-medium">Low Risk</span>
                      </>
                    ) : probability <= 40 ? (
                      <>
                        <TrendingUp className="w-4 h-4 text-yellow-600" />
                        <span className="text-yellow-700 font-medium">Moderate Risk</span>
                      </>
                    ) : (
                      <>
                        <TrendingDown className="w-4 h-4 text-red-600" />
                        <span className="text-red-700 font-medium">High Risk</span>
                      </>
                    )}
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      </CardContent>
    </Card>
  );
}
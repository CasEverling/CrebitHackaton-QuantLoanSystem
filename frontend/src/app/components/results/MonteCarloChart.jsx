import React from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Legend } from "recharts";
import { Card, CardContent } from "../../ui/card";

export default function MonteCarloChart({ data }) {
  const formatCurrency = (value) => {
    return new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: "USD",
      minimumFractionDigits: 0
    }).format(value);
  };

  const CustomTooltip = ({ active, payload, label }) => {
    if (active && payload && payload.length) {
      return (
        <div className="bg-slate-900 text-white p-4 rounded-lg shadow-xl border border-slate-700">
          <p className="font-bold mb-2">{label}</p>
          <div className="space-y-1 text-sm">
            <p className="text-green-400">
              Maximum: {formatCurrency(payload[0]?.payload?.max)}
            </p>
            <p className="text-indigo-400">
              Average: {formatCurrency(payload[0]?.payload?.avg)}
            </p>
            <p className="text-red-400">
              Minimum: {formatCurrency(payload[0]?.payload?.min)}
            </p>
          </div>
        </div>
      );
    }
    return null;
  };

  // If there's no data, show warning
  if (!data || data.length === 0) {
    return (
      <Card>
        <CardContent className="p-6">
          <div className="mb-6">
            <h3 className="text-xl font-bold text-slate-900">
              Monte Carlo Simulation - Uncertainty Range
            </h3>
            <p className="text-sm text-slate-600 mt-1">
              Simulated paths from C++ engine
            </p>
          </div>
          <div className="h-80 w-full flex items-center justify-center bg-slate-50 rounded-lg">
            <p className="text-slate-500 text-sm">⚠️ No data available for the chart</p>
          </div>
        </CardContent>
      </Card>
    );
  }

  return (
    <Card>
      <CardContent className="p-6">
        <div className="mb-6">
          <h3 className="text-xl font-bold text-slate-900">
            Monte Carlo Simulation - Uncertainty Range
          </h3>
          <p className="text-sm text-slate-600 mt-1">
            Simulated paths from C++ engine
          </p>
        </div>

        <ResponsiveContainer width="100%" height={400}>
          <AreaChart data={data} margin={{ top: 10, right: 30, left: 0, bottom: 0 }}>
            <defs>
              <linearGradient id="colorRange" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#6366f1" stopOpacity={0.3} />
                <stop offset="95%" stopColor="#6366f1" stopOpacity={0.05} />
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" stroke="#e2e8f0" />
            <XAxis
              dataKey="name"
              stroke="#64748b"
              style={{ fontSize: "12px" }}
            />
            <YAxis
              stroke="#64748b"
              style={{ fontSize: "12px" }}
              tickFormatter={(value) => formatCurrency(value)}
            />
            <Tooltip content={<CustomTooltip />} />
            <Legend />

            {/* Uncertainty area (min to max) */}
            <Area
              id="area-max"
              type="monotone"
              dataKey="max"
              stroke="#6366f1"
              strokeWidth={0}
              fill="url(#colorRange)"
              fillOpacity={1}
              name="Uncertainty Range"
            />
            <Area
              id="area-min"
              type="monotone"
              dataKey="min"
              stroke="transparent"
              strokeWidth={0}
              fill="#ffffff"
              fillOpacity={1}
              hide
            />

            {/* Average line */}
            <Area
              id="area-avg"
              type="monotone"
              dataKey="avg"
              stroke="#6366f1"
              strokeWidth={3}
              fill="none"
              name="Expected Profit"
              dot={{ fill: "#6366f1", r: 4 }}
            />
          </AreaChart>
        </ResponsiveContainer>

        <div className="mt-6 grid grid-cols-3 gap-4">
          <div className="text-center p-3 bg-green-50 rounded-lg border border-green-200">
            <p className="text-xs text-green-700 font-medium">Optimistic Scenario</p>
            <p className="text-lg font-bold text-green-800 mt-1">
              {formatCurrency(data[data.length - 1]?.max || 0)}
            </p>
          </div>
          <div className="text-center p-3 bg-indigo-50 rounded-lg border border-indigo-200">
            <p className="text-xs text-indigo-700 font-medium">Expected Scenario</p>
            <p className="text-lg font-bold text-indigo-800 mt-1">
              {formatCurrency(data[data.length - 1]?.avg || 0)}
            </p>
          </div>
          <div className="text-center p-3 bg-red-50 rounded-lg border border-red-200">
            <p className="text-xs text-red-700 font-medium">Pessimistic Scenario</p>
            <p className="text-lg font-bold text-red-800 mt-1">
              {formatCurrency(data[data.length - 1]?.min || 0)}
            </p>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}
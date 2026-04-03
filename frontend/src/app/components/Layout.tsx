// frontend/src/components/Layout.tsx
import { Link, Outlet, useLocation } from "react-router";
import { Users, Building2 } from "lucide-react";

export default function Layout() {
  const location = useLocation();

  const isActive = (path: string) => {
    return location.pathname === path;
  };

  return (
    <div className="flex h-screen bg-slate-50">
      {/* Sidebar */}
      <aside className="w-64 bg-slate-900 text-white flex flex-col">
        <div className="p-6 border-b border-slate-800">
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 bg-gradient-to-br from-indigo-500 to-purple-600 rounded-xl flex items-center justify-center">
              <Building2 className="w-6 h-6" />
            </div>
            <div>
              <h1 className="text-xl font-bold">Meridian</h1>
              <p className="text-xs text-slate-400">Credit Platform</p>
            </div>
          </div>
        </div>

        <nav className="flex-1 p-4 space-y-2">
          <Link
            to="/"
            className={`flex items-center gap-3 px-4 py-3 rounded-lg transition-colors ${
              isActive("/")
                ? "bg-indigo-600 text-white"
                : "text-slate-300 hover:bg-slate-800"
            }`}
          >
            <Users className="w-5 h-5" />
            <span>Client Portfolio</span>
          </Link>
        </nav>

        <div className="p-4 border-t border-slate-800">
          <div className="flex items-center gap-3 px-4 py-3">
            <div className="w-8 h-8 bg-indigo-500 rounded-full flex items-center justify-center text-sm font-bold">
              GM
            </div>
            <div className="text-sm">
              <p className="font-medium">Manager</p>
              <p className="text-slate-400 text-xs">admin@meridian.com</p>
            </div>
          </div>
        </div>
      </aside>

      {/* Main Content */}
      <main className="flex-1 overflow-auto">
        <Outlet />
      </main>
    </div>
  );
}
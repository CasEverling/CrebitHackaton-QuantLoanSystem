// frontend/src/components/Layout.tsx
import { Outlet, Link } from "react-router-dom";

export default function Layout() {
	return (
		<div className="flex min-h-screen bg-slate-50">
			{/* Sidebar */}
			<aside className="w-64 bg-indigo-700 text-white p-6 flex flex-col">
				<h2 className="text-xl font-black mb-6">GIG-FLOW</h2>
				<nav className="flex flex-col gap-3">
					<Link to="/" className="hover:bg-indigo-600 p-2 rounded">Análise de Cliente</Link>
					<Link to="/historico" className="hover:bg-indigo-600 p-2 rounded">Histórico</Link>
					<Link to="/contratos" className="hover:bg-indigo-600 p-2 rounded">Contratos</Link>
				</nav>
			</aside>

			{/* Área de Conteúdo */}
			<main className="flex-1 p-6">
				<Outlet />
			</main>
		</div>
	);
}
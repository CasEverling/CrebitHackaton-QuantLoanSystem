// frontend/src/App.tsx
import { BrowserRouter, Routes, Route } from "react-router-dom";
import Layout from "./components/Layout";
import ClientAnalysis from "./pages/ClientAnalysis";
import { ClientList } from "./pages/ClientList";

export default function App() {
	return (
		<BrowserRouter>
			<Routes>
				<Route path="/" element={<Layout />}>
					<Route index element={<ClientList />} />
					<Route path="analysis/:clientId" element={<ClientAnalysis />} />
					{/* Futuras rotas podem ser adicionadas aqui */}
				</Route>
			</Routes>
		</BrowserRouter>
	);
}
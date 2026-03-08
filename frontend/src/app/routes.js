// src/routes.tsx
import { createBrowserRouter } from "react-router";
import Layout from "./components/Layout";
import ClientList from "./pages/ClientList"; // Verifique se o caminho existe!
import ClientAnalysis from "./pages/ClientAnalysis";

export const router = createBrowserRouter([
  {
    path: "/",
    Component: Layout,
    children: [
      {
        index: true,
        Component: ClientList,
      },
      {
        path: "analysis/:clientId",
        Component: ClientAnalysis,
      },
    ],
  },
]);
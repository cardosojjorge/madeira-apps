import Link from "next/link";
import { cookies } from "next/headers";
import { redirect } from "next/navigation";

import { CasesTable } from "@/components/cases-table";
import { KpiCard } from "@/components/kpi-card";
import { LogoutButton } from "@/components/logout-button";
import { getSessionFromCookieStore } from "@/lib/auth";
import { formatEuro, formatPercent } from "@/lib/format";
import { getCasesForCompany, getCompanyById, getMetrics } from "@/lib/mock-data";

export default async function PortalPage() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (!session) {
    redirect("/entrar");
  }

  if (session.role !== "cliente") {
    redirect("/backoffice");
  }

  const company = getCompanyById(session.companyId);
  const cases = getCasesForCompany(session.companyId);
  const metrics = getMetrics(cases);

  return (
    <main className="mx-auto min-h-screen w-full max-w-6xl space-y-6 px-6 py-10">
      <header className="flex flex-col gap-4 rounded-2xl border border-slate-200 bg-white p-6 shadow-sm md:flex-row md:items-center md:justify-between">
        <div>
          <p className="text-xs font-semibold uppercase tracking-wide text-slate-500">Portal cliente</p>
          <h1 className="text-3xl font-black text-slate-900">{company?.nome ?? "Empresa cliente"}</h1>
          <p className="text-sm text-slate-600">
            Visao executiva para acompanhar performance de cobranca e nivel de risco.
          </p>
        </div>
        <div className="flex flex-wrap gap-2">
          <Link
            href="/"
            className="rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
          >
            Pagina comercial
          </Link>
          <LogoutButton />
        </div>
      </header>

      <section className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        <KpiCard label="Carteira total" value={formatEuro(metrics.totalCarteira)} hint="Total sob gestao" />
        <KpiCard label="Exposicao em risco" value={formatEuro(metrics.totalEmRisco)} hint="Vencidas e em plano" />
        <KpiCard
          label="Recuperado no mes"
          value={formatEuro(metrics.totalRecuperadoMes)}
          hint="Liquidadas no periodo"
        />
        <KpiCard
          label="Taxa de recuperacao"
          value={formatPercent(metrics.taxaRecuperacao)}
          hint="Recuperado / carteira total"
        />
      </section>

      <CasesTable
        title="Carteira da tua empresa"
        subtitle={`Total de ${cases.length} casos. Atualizacao em tempo real para equipa financeira.`}
        cases={cases}
      />
    </main>
  );
}

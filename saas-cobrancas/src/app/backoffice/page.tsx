import Link from "next/link";
import { cookies } from "next/headers";
import { redirect } from "next/navigation";

import { CasesTable } from "@/components/cases-table";
import { KpiCard } from "@/components/kpi-card";
import { LogoutButton } from "@/components/logout-button";
import { getSessionFromCookieStore, roleCanAccessBackoffice } from "@/lib/auth";
import { formatEuro, formatPercent } from "@/lib/format";
import { listLeads } from "@/lib/leads";
import { companies, getCasesForCompany, getMetrics } from "@/lib/mock-data";

export default async function BackofficePage() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (!session) {
    redirect("/entrar");
  }

  if (!roleCanAccessBackoffice(session.role)) {
    redirect("/portal");
  }

  const allCases = getCasesForCompany();
  const metrics = getMetrics(allCases);
  const latestLeads = listLeads(5);

  return (
    <main className="mx-auto min-h-screen w-full max-w-7xl space-y-6 px-6 py-10">
      <header className="flex flex-col gap-4 rounded-2xl border border-slate-200 bg-white p-6 shadow-sm md:flex-row md:items-center md:justify-between">
        <div>
          <p className="text-xs font-semibold uppercase tracking-wide text-slate-500">Backoffice operacional</p>
          <h1 className="text-3xl font-black text-slate-900">Centro de cobrancas CobrancaPlus</h1>
          <p className="text-sm text-slate-600">
            Sessao ativa: {session.nome} ({session.role})
          </p>
        </div>
        <div className="flex flex-wrap gap-2">
          <Link
            href="/"
            className="rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
          >
            Pagina comercial
          </Link>
          <Link
            href="/portal"
            className="rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
          >
            Ver portal cliente
          </Link>
          <LogoutButton />
        </div>
      </header>

      <section className="grid gap-4 sm:grid-cols-2 lg:grid-cols-5">
        <KpiCard label="Carteira total" value={formatEuro(metrics.totalCarteira)} hint="Todas as empresas" />
        <KpiCard label="Valor em risco" value={formatEuro(metrics.totalEmRisco)} hint="Casos vencidos + planos" />
        <KpiCard label="Recuperado no mes" value={formatEuro(metrics.totalRecuperadoMes)} hint="Pagamentos liquidados" />
        <KpiCard label="Taxa recuperacao" value={formatPercent(metrics.taxaRecuperacao)} hint="Performance consolidada" />
        <KpiCard
          label="Promessas cumpridas"
          value={formatPercent(metrics.promessasCumpridas)}
          hint="Percentagem dos casos totais"
        />
      </section>

      <section className="grid gap-4 md:grid-cols-[1.6fr,1fr]">
        <article className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
          <h2 className="text-lg font-semibold text-slate-900">Empresas ativas na carteira</h2>
          <p className="text-sm text-slate-500">Modelo pronto para revenda e operacao multi-tenant.</p>
          <div className="mt-4 space-y-3">
            {companies.map((company) => (
              <div key={company.id} className="rounded-lg border border-slate-200 p-3">
                <p className="font-semibold text-slate-900">{company.nome}</p>
                <p className="text-xs text-slate-500">
                  NIF {company.nif} • Setor {company.setor} • Plano {company.plano}
                </p>
              </div>
            ))}
          </div>
        </article>
        <article className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
          <h2 className="text-lg font-semibold text-slate-900">Leads recentes da pagina comercial</h2>
          <p className="text-sm text-slate-500">Entradas no endpoint /api/leads.</p>
          <div className="mt-4 space-y-3">
            {latestLeads.length > 0 ? (
              latestLeads.map((lead) => (
                <div key={lead.id} className="rounded-lg border border-slate-200 p-3">
                  <p className="font-semibold text-slate-900">{lead.nomeEmpresa}</p>
                  <p className="text-xs text-slate-500">{lead.nomeContacto}</p>
                  <p className="text-xs text-slate-500">{lead.email}</p>
                </div>
              ))
            ) : (
              <p className="rounded-lg border border-dashed border-slate-300 p-3 text-sm text-slate-500">
                Sem leads ainda. Testa o formulario da homepage para validar o funil comercial.
              </p>
            )}
          </div>
        </article>
      </section>

      <CasesTable
        title="Casos ativos (visao global)"
        subtitle="Fila operacional para equipa de cobranca, com prioridade por risco e atraso."
        cases={allCases}
      />
    </main>
  );
}

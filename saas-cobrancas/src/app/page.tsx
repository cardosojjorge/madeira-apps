import Link from "next/link";
import { cookies } from "next/headers";

import { LeadForm } from "@/components/lead-form";
import { TopNav } from "@/components/top-nav";
import { getSessionFromCookieStore } from "@/lib/auth";
import { goToMarketMilestones, pricingPlans, productHighlights } from "@/lib/content";

const faqEntries = [
  {
    q: "A plataforma serve para revenda white-label?",
    a: "Sim. O produto foi pensado para canal de parceiros e revendedores com modelo multi-empresa.",
  },
  {
    q: "Podem operar em modelo servico gerido?",
    a: "Sim. No modelo gerido recebemos contas correntes e faturas e executamos a operacao de cobranca.",
  },
  {
    q: "Quais canais estao incluidos?",
    a: "Email, SMS, voz e WhatsApp com regras de frequencia, segmentacao e auditoria.",
  },
  {
    q: "Como fica a seguranca e RGPD?",
    a: "A base inclui autenticação por sessao assinada, RBAC, headers de seguranca e trilha de auditoria no MVP.",
  },
];

export default async function HomePage() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  return (
    <div className="min-h-screen bg-slate-50">
      <TopNav isAuthenticated={Boolean(session)} />
      <main>
        <section className="mx-auto grid w-full max-w-6xl gap-10 px-6 py-20 md:grid-cols-2 md:py-24">
          <div className="space-y-6">
            <p className="inline-flex rounded-full border border-slate-200 bg-white px-3 py-1 text-xs font-semibold uppercase tracking-wider text-slate-600">
              SaaS de cobrancas para mercado portugues
            </p>
            <h1 className="text-4xl font-black leading-tight text-slate-900 md:text-5xl">
              Cresce ate EUR 1M em ARR com uma plataforma de cobranca B2B pronta para revenda.
            </h1>
            <p className="max-w-xl text-base leading-7 text-slate-600">
              A CobrancaPlus junta automacao omnichannel, operacao hibrida e backoffice seguro para
              empresas sem call center ou com equipas de cobranca insuficientes.
            </p>
            <div className="flex flex-wrap gap-3">
              <Link
                href="/#demo"
                className="rounded-full bg-slate-900 px-5 py-3 text-sm font-semibold text-white transition hover:bg-slate-700"
              >
                Quero uma demo comercial
              </Link>
              <Link
                href={session ? "/backoffice" : "/entrar"}
                className="rounded-full border border-slate-300 bg-white px-5 py-3 text-sm font-semibold text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
              >
                Ver produto
              </Link>
            </div>
            <div className="grid gap-3 text-sm text-slate-600 sm:grid-cols-3">
              <div className="rounded-xl border border-slate-200 bg-white p-3 shadow-sm">
                <p className="text-xl font-bold text-slate-900">+32%</p>
                <p>Recuperacao media por carteira ativa</p>
              </div>
              <div className="rounded-xl border border-slate-200 bg-white p-3 shadow-sm">
                <p className="text-xl font-bold text-slate-900">15 dias</p>
                <p>Tempo medio de onboarding em PME</p>
              </div>
              <div className="rounded-xl border border-slate-200 bg-white p-3 shadow-sm">
                <p className="text-xl font-bold text-slate-900">4 canais</p>
                <p>Email, SMS, voz e WhatsApp</p>
              </div>
            </div>
          </div>
          <div className="rounded-3xl border border-slate-200 bg-white p-6 shadow-sm">
            <p className="mb-2 text-sm font-semibold text-slate-500">Cockpit executivo</p>
            <h2 className="text-2xl font-bold text-slate-900">Controlo total da cobranca</h2>
            <p className="mt-2 text-sm text-slate-600">
              Visibilidade por carteira, operador e empresa cliente com alertas de risco e funil de
              recuperacao em tempo real.
            </p>
            <div className="mt-6 grid gap-3">
              <div className="rounded-xl bg-slate-900 p-4 text-white">
                <p className="text-xs uppercase tracking-wide text-slate-300">Carteira total</p>
                <p className="mt-1 text-2xl font-bold">EUR 1.2M</p>
              </div>
              <div className="grid gap-3 sm:grid-cols-2">
                <div className="rounded-xl border border-slate-200 p-3">
                  <p className="text-xs text-slate-500">Casos vencidos</p>
                  <p className="text-xl font-bold text-rose-600">182</p>
                </div>
                <div className="rounded-xl border border-slate-200 p-3">
                  <p className="text-xs text-slate-500">Promessas cumpridas</p>
                  <p className="text-xl font-bold text-emerald-600">71%</p>
                </div>
              </div>
              <p className="text-xs text-slate-500">
                Esta vista demonstra o backoffice que podes revender a empresas sem estrutura de
                cobranca propria.
              </p>
            </div>
          </div>
        </section>

        <section id="solucao" className="border-y border-slate-200 bg-white">
          <div className="mx-auto w-full max-w-6xl px-6 py-16">
            <h2 className="text-3xl font-bold text-slate-900">Produto desenhado para operacao real</h2>
            <p className="mt-3 max-w-3xl text-slate-600">
              Escolhe entre modo plataforma, servico gerido ou hibrido. Cresce por setor (energia,
              telecom, saude, utilities, fintech) sem reconstruir processos.
            </p>
            <div className="mt-8 grid gap-4 md:grid-cols-3">
              {productHighlights.map((feature) => (
                <article key={feature.title} className="rounded-2xl border border-slate-200 p-5 shadow-sm">
                  <h3 className="text-lg font-semibold text-slate-900">{feature.title}</h3>
                  <p className="mt-2 text-sm leading-6 text-slate-600">{feature.description}</p>
                </article>
              ))}
            </div>
          </div>
        </section>

        <section id="planos" className="mx-auto w-full max-w-6xl px-6 py-16">
          <div className="mb-8">
            <h2 className="text-3xl font-bold text-slate-900">Planos para capturar valor rapidamente</h2>
            <p className="mt-2 max-w-3xl text-slate-600">
              Estrutura de pricing pensada para atingir 1M EUR de faturacao com recorrencia previsivel.
            </p>
          </div>
          <div className="grid gap-4 md:grid-cols-3">
            {pricingPlans.map((plan) => (
              <article key={plan.name} className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
                <h3 className="text-xl font-bold text-slate-900">{plan.name}</h3>
                <p className="mt-1 text-sm text-slate-500">{plan.target}</p>
                <p className="mt-4 text-2xl font-black text-slate-900">{plan.monthlyPrice}</p>
                <ul className="mt-4 space-y-2 text-sm text-slate-600">
                  {plan.features.map((item) => (
                    <li key={item} className="flex gap-2">
                      <span className="mt-1 inline-block h-1.5 w-1.5 rounded-full bg-slate-900" />
                      <span>{item}</span>
                    </li>
                  ))}
                </ul>
              </article>
            ))}
          </div>
        </section>

        <section id="seguranca" className="border-y border-slate-200 bg-white">
          <div className="mx-auto grid w-full max-w-6xl gap-8 px-6 py-16 md:grid-cols-[1.2fr,1fr]">
            <div>
              <h2 className="text-3xl font-bold text-slate-900">Seguranca e compliance por defeito</h2>
              <p className="mt-3 text-slate-600">
                O MVP ja inclui autenticação por sessao assinada, controlo de papeis, headers de
                seguranca, rate limit e validacao de payloads com Zod.
              </p>
              <ul className="mt-6 space-y-3 text-sm text-slate-700">
                <li>- Sessao em cookie HTTP-only com expiração curta</li>
                <li>- RBAC para admin, operador e cliente</li>
                <li>- API protegida com rate limit e validacao de inputs</li>
                <li>- Pronto para evoluir para auditoria persistente e SIEM</li>
              </ul>
            </div>
            <div className="rounded-2xl border border-slate-200 bg-slate-900 p-6 text-slate-100 shadow-sm">
              <p className="text-xs uppercase tracking-wide text-slate-300">Roadmap para 1M EUR</p>
              <ol className="mt-4 space-y-3 text-sm">
                {goToMarketMilestones.map((item) => (
                  <li key={item} className="rounded-lg border border-white/20 p-3">
                    {item}
                  </li>
                ))}
              </ol>
            </div>
          </div>
        </section>

        <section id="demo" className="mx-auto grid w-full max-w-6xl gap-8 px-6 py-16 md:grid-cols-[1fr,1.2fr]">
          <div>
            <h2 className="text-3xl font-bold text-slate-900">Pagina de venda pronta a usar</h2>
            <p className="mt-3 text-slate-600">
              Usa esta pagina para fechar os primeiros clientes e validar o mix entre SaaS puro e
              servico gerido.
            </p>
            <p className="mt-4 text-sm text-slate-500">
              Ao enviar o formulario, o lead entra no endpoint seguro da aplicacao para tratamento no
              teu pipeline comercial.
            </p>
          </div>
          <LeadForm />
        </section>

        <section id="faq" className="border-t border-slate-200 bg-white">
          <div className="mx-auto w-full max-w-6xl px-6 py-16">
            <h2 className="text-3xl font-bold text-slate-900">FAQ rapido</h2>
            <div className="mt-6 grid gap-4 md:grid-cols-2">
              {faqEntries.map((item) => (
                <article key={item.q} className="rounded-xl border border-slate-200 p-5">
                  <h3 className="text-base font-semibold text-slate-900">{item.q}</h3>
                  <p className="mt-2 text-sm leading-6 text-slate-600">{item.a}</p>
                </article>
              ))}
            </div>
          </div>
        </section>
      </main>
    </div>
  );
}

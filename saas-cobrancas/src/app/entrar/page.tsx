import Link from "next/link";
import { redirect } from "next/navigation";
import { cookies } from "next/headers";

import { LoginForm } from "@/components/login-form";
import { DEMO_CREDENTIALS } from "@/lib/mock-data";
import { getSessionFromCookieStore, roleCanAccessBackoffice } from "@/lib/auth";

export default async function EntrarPage() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (session) {
    if (roleCanAccessBackoffice(session.role)) {
      redirect("/backoffice");
    }
    redirect("/portal");
  }

  return (
    <main className="mx-auto flex min-h-screen w-full max-w-6xl items-center px-6 py-16">
      <div className="grid w-full gap-8 md:grid-cols-[1fr,1.1fr]">
        <section className="space-y-5">
          <Link href="/" className="inline-flex text-sm font-semibold text-slate-600 hover:text-slate-900">
            ← Voltar para pagina comercial
          </Link>
          <h1 className="text-4xl font-black text-slate-900">Entrar na CobrancaPlus</h1>
          <p className="max-w-xl text-slate-600">
            Demo com perfis separados para validares portal cliente e backoffice operacional.
          </p>

          <div className="space-y-3 rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
            <h2 className="text-lg font-semibold text-slate-900">Credenciais demo</h2>
            <ul className="space-y-2 text-sm text-slate-600">
              <li>
                <strong>Admin:</strong> {DEMO_CREDENTIALS.admin.email} / {DEMO_CREDENTIALS.admin.password}
              </li>
              <li>
                <strong>Operador:</strong> {DEMO_CREDENTIALS.operador.email} /{" "}
                {DEMO_CREDENTIALS.operador.password}
              </li>
              <li>
                <strong>Cliente:</strong> {DEMO_CREDENTIALS.cliente.email} /{" "}
                {DEMO_CREDENTIALS.cliente.password}
              </li>
            </ul>
          </div>
        </section>

        <section>
          <LoginForm />
        </section>
      </div>
    </main>
  );
}

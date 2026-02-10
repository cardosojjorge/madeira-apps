import Link from "next/link";

interface TopNavProps {
  isAuthenticated?: boolean;
}

export function TopNav({ isAuthenticated = false }: TopNavProps) {
  return (
    <header className="sticky top-0 z-30 border-b border-slate-200/80 bg-white/90 backdrop-blur">
      <div className="mx-auto flex w-full max-w-6xl items-center justify-between px-6 py-4">
        <Link href="/" className="text-lg font-bold tracking-tight text-slate-900">
          CobrancaPlus
        </Link>
        <nav className="hidden items-center gap-6 text-sm text-slate-600 md:flex">
          <Link href="/#solucao" className="transition hover:text-slate-900">
            Solucao
          </Link>
          <Link href="/#planos" className="transition hover:text-slate-900">
            Planos
          </Link>
          <Link href="/#seguranca" className="transition hover:text-slate-900">
            Seguranca
          </Link>
          <Link href="/#faq" className="transition hover:text-slate-900">
            FAQ
          </Link>
        </nav>
        <div className="flex items-center gap-3">
          {isAuthenticated ? (
            <Link
              href="/backoffice"
              className="rounded-full border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
            >
              Abrir app
            </Link>
          ) : (
            <>
              <Link
                href="/entrar"
                className="rounded-full border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 transition hover:border-slate-900 hover:text-slate-900"
              >
                Entrar
              </Link>
              <Link
                href="/#demo"
                className="rounded-full bg-slate-900 px-4 py-2 text-sm font-semibold text-white transition hover:bg-slate-700"
              >
                Pedir demo
              </Link>
            </>
          )}
        </div>
      </div>
    </header>
  );
}

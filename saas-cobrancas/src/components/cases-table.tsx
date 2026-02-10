import type { CollectionCase } from "@/lib/types";
import { formatEuro } from "@/lib/format";

interface CasesTableProps {
  title: string;
  subtitle: string;
  cases: CollectionCase[];
}

const statusLabel: Record<CollectionCase["status"], string> = {
  em_dia: "Em dia",
  a_vencer: "A vencer",
  vencida: "Vencida",
  em_plano: "Em plano",
  liquidada: "Liquidada",
};

function statusColor(status: CollectionCase["status"]): string {
  switch (status) {
    case "em_dia":
      return "bg-emerald-50 text-emerald-700";
    case "a_vencer":
      return "bg-sky-50 text-sky-700";
    case "vencida":
      return "bg-rose-50 text-rose-700";
    case "em_plano":
      return "bg-amber-50 text-amber-700";
    case "liquidada":
      return "bg-slate-100 text-slate-700";
    default:
      return "bg-slate-100 text-slate-700";
  }
}

export function CasesTable({ title, subtitle, cases }: CasesTableProps) {
  return (
    <section className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
      <div className="mb-4">
        <h3 className="text-lg font-semibold text-slate-900">{title}</h3>
        <p className="text-sm text-slate-500">{subtitle}</p>
      </div>
      <div className="overflow-x-auto">
        <table className="min-w-full border-separate border-spacing-y-2 text-sm">
          <thead>
            <tr className="text-left text-slate-500">
              <th className="px-2 py-1">Devedor</th>
              <th className="px-2 py-1">Fatura</th>
              <th className="px-2 py-1">Valor</th>
              <th className="px-2 py-1">Atraso</th>
              <th className="px-2 py-1">Canal</th>
              <th className="px-2 py-1">Status</th>
              <th className="px-2 py-1">Ultima acao</th>
            </tr>
          </thead>
          <tbody>
            {cases.map((entry) => (
              <tr key={entry.id} className="rounded-lg bg-slate-50 text-slate-700">
                <td className="px-2 py-3">
                  <p className="font-medium text-slate-900">{entry.devedorNome}</p>
                  <p className="text-xs text-slate-500">{entry.devedorNif}</p>
                </td>
                <td className="px-2 py-3">{entry.referenciaFatura}</td>
                <td className="px-2 py-3 font-semibold text-slate-900">{formatEuro(entry.valor)}</td>
                <td className="px-2 py-3">{entry.diasAtraso} dias</td>
                <td className="px-2 py-3 uppercase">{entry.canalPreferido}</td>
                <td className="px-2 py-3">
                  <span className={`rounded-full px-2 py-1 text-xs font-semibold ${statusColor(entry.status)}`}>
                    {statusLabel[entry.status]}
                  </span>
                </td>
                <td className="px-2 py-3 text-xs text-slate-600">{entry.ultimaAcao}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}

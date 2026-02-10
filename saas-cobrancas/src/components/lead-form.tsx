"use client";

import { useState, type FormEvent } from "react";

const INITIAL_STATE = {
  nomeEmpresa: "",
  nomeContacto: "",
  email: "",
  telefone: "",
  volumeFaturasMes: "",
  modeloPreferido: "hibrido",
  mensagem: "",
};

type LeadState = typeof INITIAL_STATE;

export function LeadForm() {
  const [formState, setFormState] = useState<LeadState>(INITIAL_STATE);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [feedback, setFeedback] = useState<string | null>(null);

  async function onSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault();
    setFeedback(null);
    setIsSubmitting(true);

    try {
      const response = await fetch("/api/leads", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(formState),
      });

      if (!response.ok) {
        const payload = (await response.json()) as { message?: string };
        throw new Error(payload.message ?? "Falha ao enviar pedido.");
      }

      setFeedback("Pedido enviado com sucesso. A equipa vai contactar em ate 24h.");
      setFormState(INITIAL_STATE);
    } catch (error) {
      const message = error instanceof Error ? error.message : "Erro inesperado.";
      setFeedback(message);
    } finally {
      setIsSubmitting(false);
    }
  }

  function updateField<K extends keyof LeadState>(field: K, value: LeadState[K]) {
    setFormState((previous) => ({ ...previous, [field]: value }));
  }

  return (
    <form onSubmit={onSubmit} className="space-y-4 rounded-2xl border border-slate-200 bg-white p-6 shadow-sm">
      <div className="grid gap-4 md:grid-cols-2">
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Empresa
          <input
            required
            value={formState.nomeEmpresa}
            onChange={(event) => updateField("nomeEmpresa", event.target.value)}
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
            placeholder="Ex: Transportes Atlantico"
          />
        </label>
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Contacto
          <input
            required
            value={formState.nomeContacto}
            onChange={(event) => updateField("nomeContacto", event.target.value)}
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
            placeholder="Ex: Joao Pereira"
          />
        </label>
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Email
          <input
            required
            type="email"
            value={formState.email}
            onChange={(event) => updateField("email", event.target.value)}
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
            placeholder="contacto@empresa.pt"
          />
        </label>
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Telefone
          <input
            required
            value={formState.telefone}
            onChange={(event) => updateField("telefone", event.target.value)}
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
            placeholder="+351 912 345 678"
          />
        </label>
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Volume de faturas/mensais
          <input
            required
            value={formState.volumeFaturasMes}
            onChange={(event) => updateField("volumeFaturasMes", event.target.value)}
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
            placeholder="Ex: 5.000"
          />
        </label>
        <label className="space-y-1 text-sm font-medium text-slate-700">
          Modelo preferido
          <select
            value={formState.modeloPreferido}
            onChange={(event) =>
              updateField("modeloPreferido", event.target.value as LeadState["modeloPreferido"])
            }
            className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
          >
            <option value="hibrido">Hibrido</option>
            <option value="plataforma">Plataforma SaaS</option>
            <option value="servico_gerido">Servico gerido (BPO)</option>
          </select>
        </label>
      </div>
      <label className="space-y-1 text-sm font-medium text-slate-700">
        Mensagem
        <textarea
          rows={3}
          value={formState.mensagem}
          onChange={(event) => updateField("mensagem", event.target.value)}
          className="w-full rounded-lg border border-slate-300 px-3 py-2 text-slate-900 outline-none ring-0 transition focus:border-slate-500"
          placeholder="Descreve a tua operacao de cobranca e objetivos."
        />
      </label>
      <button
        disabled={isSubmitting}
        className="inline-flex w-full items-center justify-center rounded-lg bg-slate-900 px-4 py-3 text-sm font-semibold text-white transition hover:bg-slate-700 disabled:cursor-not-allowed disabled:opacity-70"
      >
        {isSubmitting ? "A enviar..." : "Quero uma demo"}
      </button>
      {feedback ? <p className="text-sm text-slate-700">{feedback}</p> : null}
    </form>
  );
}

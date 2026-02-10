import { hashSync } from "bcryptjs";

import type { AppUser, BackofficeMetrics, CollectionCase, Company } from "@/lib/types";

const DEMO_PASSWORD = "Demo@123";
const demoPasswordHash = hashSync(DEMO_PASSWORD, 10);

export const companies: Company[] = [
  {
    id: "cmp-porto-energy",
    nome: "Porto Energy Lda",
    nif: "517123450",
    setor: "Energia",
    plano: "enterprise",
  },
  {
    id: "cmp-saude-ativa",
    nome: "Saude Ativa S.A.",
    nif: "514667890",
    setor: "Saude",
    plano: "growth",
  },
  {
    id: "cmp-logistica-tejo",
    nome: "Logistica Tejo Unipessoal",
    nif: "508999332",
    setor: "Logistica",
    plano: "starter",
  },
];

export const users: AppUser[] = [
  {
    id: "usr-admin",
    nome: "Ines Azevedo",
    email: "admin@cobrancaplus.pt",
    passwordHash: demoPasswordHash,
    role: "admin",
  },
  {
    id: "usr-op-1",
    nome: "Rui Martins",
    email: "operador@cobrancaplus.pt",
    passwordHash: demoPasswordHash,
    role: "operador",
  },
  {
    id: "usr-cliente-1",
    nome: "Maria Costa",
    email: "cliente@portoenergy.pt",
    passwordHash: demoPasswordHash,
    role: "cliente",
    companyId: "cmp-porto-energy",
  },
];

export const collectionCases: CollectionCase[] = [
  {
    id: "case-1001",
    companyId: "cmp-porto-energy",
    devedorNome: "Henrique Silva",
    devedorNif: "298111444",
    referenciaFatura: "FAT-2025-441",
    valor: 1480.12,
    moeda: "EUR",
    dueDate: "2025-12-14",
    status: "vencida",
    diasAtraso: 44,
    canalPreferido: "whatsapp",
    ultimaAcao: "Acordo de pagamento em 3 prestacoes enviado",
    operadorResponsavel: "Rui Martins",
  },
  {
    id: "case-1002",
    companyId: "cmp-porto-energy",
    devedorNome: "Ana Ferreira",
    devedorNif: "245771222",
    referenciaFatura: "FAT-2025-500",
    valor: 312.5,
    moeda: "EUR",
    dueDate: "2026-01-28",
    status: "a_vencer",
    diasAtraso: 0,
    canalPreferido: "email",
    ultimaAcao: "Lembrete de pre-vencimento enviado",
    operadorResponsavel: "Rui Martins",
  },
  {
    id: "case-1003",
    companyId: "cmp-saude-ativa",
    devedorNome: "Clinica Nova Vida",
    devedorNif: "506733210",
    referenciaFatura: "SV-8890",
    valor: 7250,
    moeda: "EUR",
    dueDate: "2025-11-03",
    status: "em_plano",
    diasAtraso: 86,
    canalPreferido: "voz",
    ultimaAcao: "Prestacao 2/6 paga ontem",
    operadorResponsavel: "Ines Azevedo",
  },
  {
    id: "case-1004",
    companyId: "cmp-saude-ativa",
    devedorNome: "Andre Vieira",
    devedorNif: "211773001",
    referenciaFatura: "SV-9122",
    valor: 198,
    moeda: "EUR",
    dueDate: "2026-01-12",
    status: "em_dia",
    diasAtraso: 0,
    canalPreferido: "sms",
    ultimaAcao: "Plano de notificacoes automatizado ativo",
    operadorResponsavel: "Ines Azevedo",
  },
  {
    id: "case-1005",
    companyId: "cmp-logistica-tejo",
    devedorNome: "TransIberia S.L.",
    devedorNif: "ESB9923111",
    referenciaFatura: "LOG-3321",
    valor: 4390,
    moeda: "EUR",
    dueDate: "2025-10-29",
    status: "vencida",
    diasAtraso: 92,
    canalPreferido: "email",
    ultimaAcao: "Escalado para agente senior",
    operadorResponsavel: "Rui Martins",
  },
  {
    id: "case-1006",
    companyId: "cmp-porto-energy",
    devedorNome: "Sofia Rocha",
    devedorNif: "244555980",
    referenciaFatura: "FAT-2025-388",
    valor: 81.73,
    moeda: "EUR",
    dueDate: "2025-12-01",
    status: "liquidada",
    diasAtraso: 0,
    canalPreferido: "sms",
    ultimaAcao: "Pagamento recebido por MB Way",
    operadorResponsavel: "Rui Martins",
  },
  {
    id: "case-1007",
    companyId: "cmp-porto-energy",
    devedorNome: "Mercado da Vila",
    devedorNif: "506778112",
    referenciaFatura: "FAT-2025-623",
    valor: 980,
    moeda: "EUR",
    dueDate: "2025-11-18",
    status: "vencida",
    diasAtraso: 71,
    canalPreferido: "voz",
    ultimaAcao: "Contacto telefonico sem sucesso",
    operadorResponsavel: "Ines Azevedo",
  },
  {
    id: "case-1008",
    companyId: "cmp-logistica-tejo",
    devedorNome: "Construtora Horizonte",
    devedorNif: "507345882",
    referenciaFatura: "LOG-3414",
    valor: 2120,
    moeda: "EUR",
    dueDate: "2025-12-07",
    status: "em_plano",
    diasAtraso: 51,
    canalPreferido: "whatsapp",
    ultimaAcao: "Plano renegociado para 4 prestacoes",
    operadorResponsavel: "Ines Azevedo",
  },
];

export function getCasesForCompany(companyId?: string): CollectionCase[] {
  if (!companyId) {
    return collectionCases;
  }

  return collectionCases.filter((entry) => entry.companyId === companyId);
}

export function getCompanyById(companyId?: string): Company | undefined {
  return companies.find((item) => item.id === companyId);
}

export function getMetrics(cases: CollectionCase[]): BackofficeMetrics {
  const totalCarteira = cases.reduce((acc, item) => acc + item.valor, 0);
  const totalEmRisco = cases
    .filter((item) => item.status === "vencida" || item.status === "em_plano")
    .reduce((acc, item) => acc + item.valor, 0);
  const totalRecuperadoMes = cases
    .filter((item) => item.status === "liquidada")
    .reduce((acc, item) => acc + item.valor, 0);
  const totalAcoes = cases.length;
  const promessasCumpridas = cases.filter((item) => item.status === "em_plano").length;
  const taxaRecuperacao =
    totalCarteira > 0 ? Number(((totalRecuperadoMes / totalCarteira) * 100).toFixed(2)) : 0;

  return {
    totalCarteira: Number(totalCarteira.toFixed(2)),
    totalEmRisco: Number(totalEmRisco.toFixed(2)),
    totalRecuperadoMes: Number(totalRecuperadoMes.toFixed(2)),
    taxaRecuperacao,
    promessasCumpridas: totalAcoes > 0 ? Number(((promessasCumpridas / totalAcoes) * 100).toFixed(2)) : 0,
  };
}

export const DEMO_CREDENTIALS = {
  admin: {
    email: "admin@cobrancaplus.pt",
    password: DEMO_PASSWORD,
  },
  operador: {
    email: "operador@cobrancaplus.pt",
    password: DEMO_PASSWORD,
  },
  cliente: {
    email: "cliente@portoenergy.pt",
    password: DEMO_PASSWORD,
  },
};

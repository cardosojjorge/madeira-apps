export type UserRole = "admin" | "operador" | "cliente";

export type InvoiceStatus =
  | "em_dia"
  | "a_vencer"
  | "vencida"
  | "em_plano"
  | "liquidada";

export type ContactChannel = "email" | "sms" | "voz" | "whatsapp";

export interface Company {
  id: string;
  nome: string;
  nif: string;
  setor: string;
  plano: "starter" | "growth" | "enterprise";
}

export interface CollectionCase {
  id: string;
  companyId: string;
  devedorNome: string;
  devedorNif: string;
  referenciaFatura: string;
  valor: number;
  moeda: "EUR";
  dueDate: string;
  status: InvoiceStatus;
  diasAtraso: number;
  canalPreferido: ContactChannel;
  ultimaAcao: string;
  operadorResponsavel: string;
}

export interface BackofficeMetrics {
  totalCarteira: number;
  totalEmRisco: number;
  totalRecuperadoMes: number;
  taxaRecuperacao: number;
  promessasCumpridas: number;
}

export interface AppUser {
  id: string;
  nome: string;
  email: string;
  passwordHash: string;
  role: UserRole;
  companyId?: string;
}

export interface SessionPayload extends Record<string, unknown> {
  userId: string;
  nome: string;
  email: string;
  role: UserRole;
  companyId?: string;
}

export interface LeadRequest {
  nomeEmpresa: string;
  nomeContacto: string;
  email: string;
  telefone: string;
  volumeFaturasMes: string;
  modeloPreferido: "hibrido" | "plataforma" | "servico_gerido";
  mensagem?: string;
}

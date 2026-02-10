import { NextResponse } from "next/server";
import { cookies } from "next/headers";
import { z } from "zod";

import { getSessionFromCookieStore, roleCanAccessBackoffice } from "@/lib/auth";
import { addLead, listLeads } from "@/lib/leads";

const leadSchema = z.object({
  nomeEmpresa: z.string().trim().min(2).max(120),
  nomeContacto: z.string().trim().min(2).max(120),
  email: z.string().trim().email().max(120),
  telefone: z.string().trim().min(9).max(20),
  volumeFaturasMes: z.string().trim().min(1).max(40),
  modeloPreferido: z.enum(["hibrido", "plataforma", "servico_gerido"]),
  mensagem: z.string().trim().max(600).optional(),
});

export async function POST(request: Request) {
  let jsonBody: unknown;

  try {
    jsonBody = await request.json();
  } catch {
    return NextResponse.json({ message: "Payload invalido." }, { status: 400 });
  }

  const parsedPayload = leadSchema.safeParse(jsonBody);
  if (!parsedPayload.success) {
    return NextResponse.json(
      { message: "Dados de lead invalidos.", issues: parsedPayload.error.issues },
      { status: 400 },
    );
  }

  const record = addLead(parsedPayload.data);
  return NextResponse.json({ message: "Lead criado.", id: record.id }, { status: 201 });
}

export async function GET() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (!session || !roleCanAccessBackoffice(session.role)) {
    return NextResponse.json({ message: "Acesso negado." }, { status: 403 });
  }

  return NextResponse.json({ data: listLeads(30) });
}

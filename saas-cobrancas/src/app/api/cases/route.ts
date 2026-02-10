import { NextResponse } from "next/server";
import { cookies } from "next/headers";
import { z } from "zod";

import { getSessionFromCookieStore } from "@/lib/auth";
import { getCasesForCompany, getMetrics } from "@/lib/mock-data";

const querySchema = z.object({
  status: z.enum(["em_dia", "a_vencer", "vencida", "em_plano", "liquidada"]).optional(),
});

export async function GET(request: Request) {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (!session) {
    return NextResponse.json({ message: "Nao autenticado." }, { status: 401 });
  }

  const currentUrl = new URL(request.url);
  const parsedQuery = querySchema.safeParse({
    status: currentUrl.searchParams.get("status") ?? undefined,
  });

  if (!parsedQuery.success) {
    return NextResponse.json({ message: "Filtro invalido." }, { status: 400 });
  }

  const baseCases = session.role === "cliente" ? getCasesForCompany(session.companyId) : getCasesForCompany();

  const filteredCases = parsedQuery.data.status
    ? baseCases.filter((item) => item.status === parsedQuery.data.status)
    : baseCases;

  return NextResponse.json({
    count: filteredCases.length,
    metrics: getMetrics(filteredCases),
    data: filteredCases,
  });
}

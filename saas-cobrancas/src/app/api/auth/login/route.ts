import { NextResponse } from "next/server";
import { z } from "zod";

import {
  authenticateUser,
  createSessionToken,
  SESSION_COOKIE_NAME,
  SESSION_DURATION_SECONDS,
} from "@/lib/auth";

export const runtime = "nodejs";

const loginSchema = z.object({
  email: z.string().trim().email().max(120),
  password: z.string().min(8).max(72),
});

export async function POST(request: Request) {
  let jsonBody: unknown;

  try {
    jsonBody = await request.json();
  } catch {
    return NextResponse.json({ message: "Payload invalido." }, { status: 400 });
  }

  const parsedPayload = loginSchema.safeParse(jsonBody);
  if (!parsedPayload.success) {
    return NextResponse.json(
      { message: "Formato de login invalido.", issues: parsedPayload.error.issues },
      { status: 400 },
    );
  }

  const { email, password } = parsedPayload.data;
  const user = await authenticateUser(email, password);

  if (!user) {
    return NextResponse.json({ message: "Credenciais invalidas." }, { status: 401 });
  }

  const sessionToken = await createSessionToken({
    userId: user.id,
    nome: user.nome,
    email: user.email,
    role: user.role,
    companyId: user.companyId,
  });

  const response = NextResponse.json({
    message: "Login efetuado com sucesso.",
    role: user.role,
  });

  response.cookies.set({
    name: SESSION_COOKIE_NAME,
    value: sessionToken,
    httpOnly: true,
    secure: process.env.NODE_ENV === "production",
    sameSite: "strict",
    path: "/",
    maxAge: SESSION_DURATION_SECONDS,
  });

  return response;
}

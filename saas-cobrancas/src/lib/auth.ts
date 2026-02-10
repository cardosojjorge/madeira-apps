import { compare } from "bcryptjs";
import { jwtVerify, SignJWT } from "jose";

import { users } from "@/lib/mock-data";
import type { AppUser, SessionPayload, UserRole } from "@/lib/types";

export const SESSION_COOKIE_NAME = "cbp_session";
export const SESSION_DURATION_SECONDS = 60 * 60 * 8;

type CookieStoreLike = {
  get: (name: string) => { value: string } | undefined;
};

function getJwtSecret(): Uint8Array {
  const value =
    process.env.JWT_SECRET ??
    "change-me-in-production-please-use-a-long-secret-with-32-characters";

  return new TextEncoder().encode(value);
}

export async function createSessionToken(payload: SessionPayload): Promise<string> {
  return new SignJWT(payload)
    .setProtectedHeader({ alg: "HS256" })
    .setIssuedAt()
    .setExpirationTime(`${SESSION_DURATION_SECONDS}s`)
    .sign(getJwtSecret());
}

export async function verifySessionToken(token?: string | null): Promise<SessionPayload | null> {
  if (!token) {
    return null;
  }

  try {
    const { payload } = await jwtVerify(token, getJwtSecret());
    return {
      userId: String(payload.userId),
      nome: String(payload.nome),
      email: String(payload.email),
      role: payload.role as UserRole,
      companyId: payload.companyId ? String(payload.companyId) : undefined,
    };
  } catch {
    return null;
  }
}

export async function getSessionFromCookieStore(
  cookieStore: CookieStoreLike,
): Promise<SessionPayload | null> {
  const token = cookieStore.get(SESSION_COOKIE_NAME)?.value;
  return verifySessionToken(token);
}

export async function authenticateUser(email: string, password: string): Promise<AppUser | null> {
  const user = users.find((entry) => entry.email.toLowerCase() === email.toLowerCase());
  if (!user) {
    return null;
  }

  const passwordMatches = await compare(password, user.passwordHash);
  if (!passwordMatches) {
    return null;
  }

  return user;
}

export function roleCanAccessBackoffice(role: UserRole): boolean {
  return role === "admin" || role === "operador";
}

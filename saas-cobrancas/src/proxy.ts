import { NextResponse, type NextRequest } from "next/server";

import { SESSION_COOKIE_NAME, verifySessionToken } from "@/lib/auth";

const AUTH_PAGES = ["/portal", "/backoffice"];
const PROTECTED_API_PREFIXES = ["/api/session", "/api/cases", "/api/leads", "/api/auth/logout"];

const RATE_LIMIT_WINDOW_MS = 60_000;
const RATE_LIMIT_MAX_REQUESTS = 100;
const requestCounter = new Map<string, { count: number; resetAt: number }>();

function shouldProtectApiPath(pathname: string, method: string): boolean {
  if (pathname === "/api/leads" && method === "POST") {
    return false;
  }
  return PROTECTED_API_PREFIXES.some((prefix) => pathname.startsWith(prefix));
}

function applySecurityHeaders(response: NextResponse): NextResponse {
  response.headers.set("X-Frame-Options", "DENY");
  response.headers.set("X-Content-Type-Options", "nosniff");
  response.headers.set("Referrer-Policy", "strict-origin-when-cross-origin");
  response.headers.set("Permissions-Policy", "camera=(), microphone=(), geolocation=()");
  response.headers.set(
    "Content-Security-Policy",
    "default-src 'self'; img-src 'self' data:; script-src 'self'; style-src 'self' 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'self'; form-action 'self'",
  );
  return response;
}

function enforceRateLimit(request: NextRequest): NextResponse | null {
  if (!request.nextUrl.pathname.startsWith("/api/")) {
    return null;
  }

  const ipAddress = request.headers.get("x-forwarded-for")?.split(",")[0].trim() ?? "unknown";
  const key = `${ipAddress}:${request.nextUrl.pathname}`;
  const now = Date.now();
  const state = requestCounter.get(key);

  if (!state || state.resetAt <= now) {
    requestCounter.set(key, { count: 1, resetAt: now + RATE_LIMIT_WINDOW_MS });
    return null;
  }

  state.count += 1;
  if (state.count > RATE_LIMIT_MAX_REQUESTS) {
    return NextResponse.json({ message: "Demasiados pedidos. Tenta novamente em 1 minuto." }, { status: 429 });
  }

  return null;
}

export async function proxy(request: NextRequest) {
  const rateLimitResponse = enforceRateLimit(request);
  if (rateLimitResponse) {
    return applySecurityHeaders(rateLimitResponse);
  }

  const pathname = request.nextUrl.pathname;
  const requiresPageAuth = AUTH_PAGES.some((prefix) => pathname.startsWith(prefix));
  const requiresApiAuth = shouldProtectApiPath(pathname, request.method);

  if (!requiresPageAuth && !requiresApiAuth) {
    return applySecurityHeaders(NextResponse.next());
  }

  const token = request.cookies.get(SESSION_COOKIE_NAME)?.value;
  const session = await verifySessionToken(token);

  if (!session) {
    if (requiresApiAuth) {
      return applySecurityHeaders(NextResponse.json({ message: "Nao autenticado." }, { status: 401 }));
    }

    const loginUrl = new URL("/entrar", request.url);
    loginUrl.searchParams.set("from", pathname);
    return applySecurityHeaders(NextResponse.redirect(loginUrl));
  }

  if (pathname.startsWith("/backoffice") && session.role === "cliente") {
    return applySecurityHeaders(NextResponse.redirect(new URL("/portal", request.url)));
  }

  if (pathname.startsWith("/portal") && session.role !== "cliente") {
    return applySecurityHeaders(NextResponse.redirect(new URL("/backoffice", request.url)));
  }

  if (pathname.startsWith("/api/leads") && request.method !== "POST") {
    if (session.role === "cliente") {
      return applySecurityHeaders(NextResponse.json({ message: "Acesso negado." }, { status: 403 }));
    }
  }

  return applySecurityHeaders(NextResponse.next());
}

export const config = {
  matcher: ["/((?!_next/static|_next/image|favicon.ico|sitemap.xml|robots.txt).*)"],
};

import { NextResponse } from "next/server";
import { cookies } from "next/headers";

import { getSessionFromCookieStore } from "@/lib/auth";

export async function GET() {
  const cookieStore = await cookies();
  const session = await getSessionFromCookieStore(cookieStore);

  if (!session) {
    return NextResponse.json({ authenticated: false }, { status: 401 });
  }

  return NextResponse.json({
    authenticated: true,
    session,
  });
}

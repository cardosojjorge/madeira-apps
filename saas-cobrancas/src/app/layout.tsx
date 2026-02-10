import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import "./globals.css";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin"],
});

const geistMono = Geist_Mono({
  variable: "--font-geist-mono",
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: {
    default: "CobrancaPlus | SaaS de cobrancas para Portugal",
    template: "%s | CobrancaPlus",
  },
  description:
    "Plataforma SaaS de gestao de cobrancas para empresas portuguesas: operacao hibrida, omnichannel e backoffice com seguranca.",
  keywords: [
    "saas cobrancas",
    "gestao cobrancas portugal",
    "plataforma cobranca",
    "backoffice cobranca",
    "colecao de divida b2b",
  ],
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="pt-PT">
      <body
        className={`${geistSans.variable} ${geistMono.variable} bg-slate-50 antialiased`}
      >
        {children}
      </body>
    </html>
  );
}

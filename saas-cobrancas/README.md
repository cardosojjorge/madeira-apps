# CobrancaPlus - SaaS de cobrancas para mercado portugues

MVP funcional para uma plataforma SaaS de cobrancas com:

- **Pagina comercial** para vender o produto (`/`)
- **Login seguro** com sessao assinada (`/entrar`)
- **Portal cliente** com visao de carteira (`/portal`)
- **Backoffice operacional** para equipa de cobranca (`/backoffice`)
- **APIs protegidas** com validacao, RBAC e rate limit basico

## Stack

- Next.js 16 (App Router)
- TypeScript
- Tailwind CSS v4
- Zod (validacao)
- jose (JWT)
- bcryptjs (password hash)

## Como executar

```bash
npm install
npm run dev
```

Abrir: `http://localhost:3000`

## Credenciais demo

Password para todos: `Demo@123`

- Admin: `admin@cobrancaplus.pt`
- Operador: `operador@cobrancaplus.pt`
- Cliente: `cliente@portoenergy.pt`

## Rotas principais

- `GET /` pagina comercial e formulario de demo
- `GET /entrar` autenticacao
- `GET /portal` portal cliente (role cliente)
- `GET /backoffice` painel operacional (roles admin/operador)
- `GET /api/cases` listagem de casos filtrada por role
- `POST /api/leads` captura de leads da pagina de venda
- `GET /api/leads` leitura de leads (admin/operador)
- `GET /api/session` sessao atual

## Variaveis de ambiente recomendadas

Criar `.env.local`:

```env
JWT_SECRET=trocar-por-secret-forte-com-32-caracteres-ou-mais
```

## Documentacao adicional

- `docs/arquitetura-produto.md` - arquitetura, seguranca e roadmap
- `docs/perguntas-estrategicas.md` - perguntas para afinar oferta e GTM

## Nota importante

Este repositorio e um MVP comercial e tecnico para validacao rapida.  
Para producao, recomenda-se adicionar:

- base de dados persistente (PostgreSQL)
- logs/auditoria imutavel
- observabilidade (SIEM, traces)
- pipeline CI/CD com SAST/DAST

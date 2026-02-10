# Arquitetura de produto - CobrancaPlus

## 1) Proposta

A CobrancaPlus foi desenhada para dois modelos:

1. **Plataforma SaaS**: cliente usa software para gerir cobranca internamente.
2. **Servico gerido (BPO)**: cliente envia contas correntes/faturas e a operacao e executada pela CobrancaPlus.
3. **Modelo hibrido**: parte automatizada no cliente + parte operada por equipa externa.

## 2) Segmentos alvo em Portugal

- Energia e utilities
- Saude privada
- Logistica e distribuicao
- Telecom e servicos recorrentes
- Fintech e credito ao consumo (com compliance reforcado)

## 3) Capas da solucao

### Front-office (cliente final)

- Landing page de venda
- Formulario de demo
- Portal cliente com KPI e visibilidade por carteira

### Backoffice (operacao cobranca)

- Cockpit de casos
- Fila operacional por risco/atraso
- Gestao multi-empresa para revenda

### API e seguranca

- Sessao em cookie HTTP-only com JWT assinado
- RBAC: admin, operador, cliente
- Validacao de payloads com Zod
- Headers de seguranca e rate limit no middleware

## 4) Compliance e requisitos PT/EU (base)

- RGPD: minimizacao de dados, retention policy, logging de acesso
- DPA com clientes B2B e subprocessadores
- Processo de resposta a pedidos de titulares
- Historico de contactos e consentimento por canal (quando aplicavel)

## 5) Roadmap tecnico para escalar a EUR 1M

### Fase A (0-3 meses)

- MVP comercial (ja incluido)
- Primeiras integracoes ERP via API
- Dashboard de receita e churn

### Fase B (3-9 meses)

- Multi-tenant com isolamento forte por empresa
- PostgreSQL + auditoria persistente
- Motor de regras por carteira (SLA, prioridade, scripts)

### Fase C (9-18 meses)

- Scoring preditivo de cobranca
- Workforce management para operadores
- Canal parceiros (white-label, comissoes, portal de revenda)

## 6) Modelo economico indicativo para atingir EUR 1M ARR

Combinacao sugerida:

- 20 clientes Growth (~EUR 1.990/m) = ~EUR 478k/ano
- 10 clientes Enterprise (~EUR 3.900/m medio) = ~EUR 468k/ano
- Servicos de onboarding/projetos = ~EUR 80k+/ano

Total indicativo: > EUR 1M/ano.

## 7) Riscos principais e mitigacao

- **Risco comercial**: ciclo de venda longo -> mitigar com pilotos de 30-45 dias.
- **Risco operacional**: dependencia de equipa humana -> automacao por regras e IA assistiva.
- **Risco legal/compliance**: canais e dados sensiveis -> DPO, auditoria, processos internos.

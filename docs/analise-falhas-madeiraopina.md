# Análise de falhas — madeiraopina.com

**URL:** https://www.madeiraopina.com  
**Data:** 2026-08-03  
**Stack:** Blogger (Google) + tema **News Blogger / Piki Templates** v1.3.0.V  
**Âmbito:** falhas de template/tema vs falhas de CMS/conteúdo

---

## Veredicto

O site está operacional, mas tem **falhas recorrentes de conteúdo (CMS)** e **falhas de localização/configuração do tema**. O problema mais visível — palavras partidas tipo `N ão`, `H á`, `E sta` — **não é um bug de CSS do drop cap no artigo**; é o resultado da forma como as letras capitulares são inseridas no editor + o gerador de resumos do Blogger.

| Origem | Gravidade | Exemplos |
|--------|-----------|----------|
| CMS / conteúdo editorial | Alta | Drop caps mal estruturados → resumos partidos; URLs Facebook como corpo; `&nbsp;` no texto |
| Template / opções do tema | Alta | UI em inglês; truncagem de entidades HTML nos snippets; FB SDK mal configurado |
| Infra / anúncios | Média | Recursos 404 / DNS de redes publicitárias |

---

## 1. Falhas de CMS / conteúdo

### 1.1 Letras capitulares partidas nos resumos (crítico)

**Sintoma:** nos snippets da homepage (`post-snippet`) aparecem `N ão`, `H á`, `E sta`, `S e`, `V im`, `U ma`, `N a`.

**Quantificação (homepage, 20 snippets):** 12/20 com letra capital espaçada.

**Causa (confirmada no HTML dos posts):** a primeira letra é colocada num `<p>` isolado com `float:left`, e o resto da palavra noutro bloco:

```html
<p>
  <span style="color:#8c0c84; float:left; font-family:Georgia, serif;
               font-size:50px; font-weight:bold; ...">N</span>
</p>
<div style="text-align:justify;">ão é sátira. Aconteceu. ...</div>
```

No artigo, o `float` junta visualmente `N` + `ão`.  
No resumo, o Blogger remove tags e junta blocos com espaço → **`N ão`**.

**No artigo o drop cap parece correto** (ex.: [Miguel Albuquerque recomenda livros](https://www.madeiraopina.com/2026/08/miguel-albuquerque-recomenda-livros.html)); o estrago é nos listados/resumos/SEO text.

**Correção recomendada (CMS):**
1. Manter a letra **inline** no mesmo parágrafo do resto da palavra.
2. Preferir o shortcode do tema: `<strike>(caps)</strike>` letra/palavra `</strike>` → gera `<span class="firstword">`, sem partir o bloco.
3. Evitar `</p>` entre a letra e o resto da palavra.
4. Regravar posts já publicados afetados (ou script de limpeza no HTML dos posts).

---

### 1.2 URLs de Facebook como conteúdo (crítico)

**Exemplos:**
- [Finalmente um Padre…](https://www.madeiraopina.com/2026/08/finalmente-um-padre-que-fala-como.html) — o post começa com `<ul><li>https://www.facebook.com/...</li></ul>`
- [Raio-X à Igreja da Madeira](https://www.madeiraopina.com/2026/08/raio-x-igreja-da-madeira.html) — URL Facebook no excerpt

O resumo da homepage fica só com o URL truncado, em vez de texto legível.

**Correção:** mover o link para hiperligação com texto âncora (ou embed), e garantir que o **primeiro parágrafo legível** é texto jornalístico (é isso que o Blogger usa no snippet).

---

### 1.3 `&nbsp;` / `&#160;` no meio do texto (médio)

Vários posts têm espaços não-separáveis no corpo (`A &#160;gestão`, `O&#160; &#160;DN-M`, `É &#160;lamentável`). No artigo quase não se nota; no snippet aparece lixo/`&#160;` e espaços estranhos.

**Causa típica:** colar de Word/Docs/Facebook ou formatação “justificar” no editor do Blogger.

**Correção:** limpar `&nbsp;` desnecessários no HTML dos posts; colar como texto simples.

---

### 1.4 Aspas e entidades cortadas nos excerpts (misto CMS + template)

Exemplos reais nos snippets:

- `turista&quot&#8230;` (entidade `&quot;` cortada a meio + reticências)
- `António&qu&#8230;`

Há conteúdo com aspas tipográficas/`&quot;` no corpo; o truncador do tema/Blogger corta a meio da entidade.

**Correção CMS:** preferir aspas tipográficas Unicode (`“ ”`) ou aspas simples no texto.  
**Correção template:** truncar por caracteres de texto descodificado, nunca a meio de uma entidade HTML (ver §2.2).

---

## 2. Falhas de template / tema (Piki)

### 2.1 Interface em inglês num site pt-PT (alto)

| Local | Texto atual | Deveria |
|-------|-------------|---------|
| Overlay de pesquisa | `Type Here to Get Search Results !` | ex. `Escreva para pesquisar` |
| Placeholder pesquisa | `Search Here...` | `Pesquisar...` |
| Secção listagem | `Read more »` | `Ler mais »` |
| Botão AJAX | `Load More` (var `loadMorePosts`) | `Carregar mais` |
| Fallback categorias | `Uncategorized` | `Sem categoria` |
| TOC shortcode | `Table of Contents` | `Índice` |
| Byline schema | `Comments` / `Tags` | `Comentários` / `Etiquetas` |

Parte das mensagens já está em PT (`pikiMessages.showMore = "Mostrar mais"`, meses em português, `relatedPostsText = "Poderá gostar também:"`), mas várias strings do XML do tema / painel Admin ficaram por traduzir.

**Onde alterar (Blogger):** Tema → Editar HTML / widgets **Theme Options (Admin Panel)** (`TextList` / `LinkList` com variáveis JS), e o markup do search overlay.

---

### 2.2 Truncagem insegura de snippets (alto)

Os `<p class='post-snippet'>` cortam com `&#8230;` sem respeitar entidades HTML completas → `&quot&#8230;`.

**Correção no tema:** ao gerar/limitar excerpt, fazer `html_entity_decode` (ou equivalente), truncar texto puro, depois escapar de novo — ou truncar só em boundaries seguros.

---

### 2.3 Facebook SDK / plugins mal apontados (médio)

No painel admin do tema:

```html
<script ... src='https://www.facebook.com/MadeiraOpina'></script>
```

Isto **não** é o SDK do Facebook (`connect.facebook.net/.../sdk.js`); é a URL da página. O script falha / não inicializa comentários/partilhas FB corretamente.

`disqusShortname = "pikitemplates"` permanece o valor de demo do tema (inofensivo enquanto `commentsSystem = "blogger"`, mas é configuração residual).

---

### 2.4 Placeholder de imagem e ads (médio/baixo)

- `noThumb` aponta para um URL antigo `bp.blogspot.com` de demo do tema.
- Consola do browser: dezenas/centenas de falhas de recursos de redes de ads (`ERR_NAME_NOT_RESOLVED`, 404 de gifs/trackers) — impacto em performance e ruído, não no layout editorial principal.

---

### 2.5 Feed `/feed/` 404 (baixo)

- `https://www.madeiraopina.com/feeds/posts/default` → **200**
- `https://www.madeiraopina.com/feeds/posts/default?alt=rss` → **200**
- `https://www.madeiraopina.com/feed/` → **404** (rota WordPress; este site é Blogger)

Só é problema se algum serviço externo estiver configurado para `/feed/`.

---

## 3. O que está bem

- Homepage, navegação, categorias, artigos e footer renderizam de forma utilizável.
- Drop cap **no corpo do artigo** (float) apresenta-se visualmente aceitável.
- Comentários Blogger ativos (`0 Comentários` / `Enviar um comentário`).
- Feeds Atom/RSS nativos do Blogger respondem.
- Alguma i18n do tema já em português (meses, “Mostrar mais”, relacionados).

---

## 4. Prioridade de correção

1. **CMS:** corrigir markup das letras capitulares (letra + resto no mesmo `<p>` / shortcode `(caps)`).
2. **CMS:** remover URLs cruas do início dos posts; garantir primeiro parágrafo útil.
3. **Template:** traduzir strings EN do search / Read more / Load More.
4. **Template:** truncar snippets sem partir entidades HTML.
5. **Template:** corrigir script do Facebook SDK; limpar `disqusShortname` / `noThumb` de demo.
6. **CMS:** limpar `&nbsp;` e colagens sujas nos posts recentes.
7. **Ops:** rever tags/slots de ads que geram 404/DNS errors.

---

## 5. Evidências

Capturas em `/opt/cursor/artifacts/madeiraopina-analise/`:

| Ficheiro | O quê |
|----------|-------|
| `01-snippet-letra-quebrada.webp` | Resumo com `N ão` |
| `02-pesquisa-ingles.webp` | Overlay de pesquisa em inglês |
| `03-dropcap-artigo-ok.webp` | Drop cap correto no artigo |
| `04-url-facebook-conteudo.webp` | URL Facebook como primeiro conteúdo |

**Amostra de snippets problemáticos (homepage):**

```
N ão é sátira...
E sta será a minha última resposta...
V im porque uma família...
H á um velho provérbio...
S e o Rali Madeira...
U ma menina de oito anos...
turista&quot&#8230;
António&qu&#8230;
https://www.facebook.com/joseluis.rodrigues.12/posts/...
```

---

## 6. Nota sobre este repositório

`madeira-apps` não contém o XML do tema nem acesso ao CMS Blogger. As correções 1–6 fazem-se em:

1. **Blogger → Publicações** (HTML dos posts)  
2. **Blogger → Tema → Editar HTML** / widgets de opções Piki  

Se no futuro o XML do tema for versionado aqui, as correções de template (§2) podem ser feitas por PR neste repo.

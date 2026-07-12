# Fuente de la Wiki

Esta carpeta contiene las paginas Markdown de la wiki de GitHub.

La pagina inicial es `Home.md` y el menu lateral es `_Sidebar.md`.

Para publicarlas en la pestana **Wiki** del repositorio, ejecuta desde la raiz:

```bash
./scripts/publish-wiki.sh
```

GitHub Wiki usa un repositorio separado (`Physarum-Automata.wiki.git`), por eso no basta con hacer push del repositorio principal.

Si agregas una pagina nueva:

1. crea el archivo Markdown en esta carpeta;
2. agrega el enlace en `Home.md`;
3. agrega el enlace en `_Sidebar.md`;
4. ejecuta `./scripts/publish-wiki.sh` cuando quieras publicarla.

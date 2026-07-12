# Publicar en GitHub Wiki

## Importante

La pestana **Wiki** de GitHub no lee automaticamente la carpeta `wiki/` del repositorio principal.

GitHub Wiki usa otro repositorio Git con este formato:

```text
git@github.com:USUARIO/REPOSITORIO.wiki.git
```

Para este proyecto:

```text
git@github.com:Jessenox/Physarum-Automata.wiki.git
```

Por eso, hacer `git push origin main` solo sube el codigo y los archivos fuente de la wiki, pero no publica la pestana Wiki.

## Flujo recomendado

1. Edita los archivos Markdown dentro de `wiki/`.
2. Sube esos cambios al repositorio principal.
3. Ejecuta el script de publicacion:

```bash
./scripts/publish-wiki.sh
```

4. Abre la pestana **Wiki** en GitHub.

## Requisitos

- Tener habilitada la Wiki del repositorio en GitHub.
- Tener permisos de escritura sobre `Jessenox/Physarum-Automata`.
- Tener configurada la llave SSH que permite hacer push a GitHub.

## Que hace el script

El script:

- detecta el remoto `origin`;
- calcula el remoto `.wiki.git`;
- clona la wiki en una carpeta temporal;
- copia el contenido de `wiki/`;
- crea un commit si hay cambios;
- hace push a la wiki.

## Si GitHub dice que el repo wiki no existe

En GitHub:

1. entra al repositorio;
2. abre **Settings**;
3. busca **Features**;
4. habilita **Wikis**;
5. vuelve a ejecutar `./scripts/publish-wiki.sh`.


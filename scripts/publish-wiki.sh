#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WIKI_SOURCE_DIR="$ROOT_DIR/wiki"
origin_url=""
wiki_url=""

usage() {
  cat <<'EOF'
Uso:
  ./scripts/publish-wiki.sh
  ./scripts/publish-wiki.sh --remote git@github.com:OWNER/REPO.git
  ./scripts/publish-wiki.sh --wiki-url git@github.com:OWNER/REPO.wiki.git

Notas:
  - GitHub Wiki usa un repositorio separado terminado en .wiki.git.
  - Si GitHub responde "Repository not found", habilita Wikis en Settings
    del repositorio y crea la primera pagina desde la pestana Wiki.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --remote)
      if [[ $# -lt 2 ]]; then
        echo "Falta valor para --remote." >&2
        exit 1
      fi
      origin_url="$2"
      shift 2
      ;;
    --wiki-url)
      if [[ $# -lt 2 ]]; then
        echo "Falta valor para --wiki-url." >&2
        exit 1
      fi
      wiki_url="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Argumento no reconocido: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ ! -d "$WIKI_SOURCE_DIR" ]]; then
  echo "No existe la carpeta wiki/: $WIKI_SOURCE_DIR" >&2
  exit 1
fi

if [[ -z "$origin_url" ]]; then
  origin_url="$(git -C "$ROOT_DIR" remote get-url origin)"
fi

if [[ -z "$wiki_url" ]]; then
  case "$origin_url" in
    git@github.com:*.git)
      wiki_url="${origin_url%.git}.wiki.git"
      ;;
    git@github.com:*)
      wiki_url="${origin_url}.wiki.git"
      ;;
    https://github.com/*.git)
      wiki_url="${origin_url%.git}.wiki.git"
      ;;
    https://github.com/*)
      wiki_url="${origin_url}.wiki.git"
      ;;
    *)
      echo "No puedo derivar el remoto wiki desde origin: $origin_url" >&2
      exit 1
      ;;
  esac
fi

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

echo "Publicando wiki en: $wiki_url"

if ! git clone "$wiki_url" "$tmp_dir/wiki-repo"; then
  cat >&2 <<EOF
No se pudo clonar la wiki.

El remoto calculado fue:
  $wiki_url

Si GitHub muestra "Repository not found", normalmente significa una de estas cosas:
  1. La opcion Wikis no esta habilitada en el repositorio.
  2. La wiki aun no fue inicializada con una primera pagina.
  3. Tu llave SSH no tiene permisos de escritura sobre la wiki.
  4. El repo esta en otro owner/nombre y debes pasar --remote o --wiki-url.

Para inicializarla:
  1. Abre https://github.com/Jessenox/Physarum-Automata/settings
  2. En Features, habilita Wikis.
  3. Abre https://github.com/Jessenox/Physarum-Automata/wiki
  4. Crea una pagina inicial cualquiera y guardala.
  5. Ejecuta de nuevo ./scripts/publish-wiki.sh

Si quieres usar un remoto explicito:
  ./scripts/publish-wiki.sh --remote git@github.com:Jessenox/Physarum-Automata.git
  ./scripts/publish-wiki.sh --wiki-url git@github.com:Jessenox/Physarum-Automata.wiki.git
EOF
  exit 1
fi

find "$tmp_dir/wiki-repo" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
cp -R "$WIKI_SOURCE_DIR"/. "$tmp_dir/wiki-repo"/

git -C "$tmp_dir/wiki-repo" add --all

if git -C "$tmp_dir/wiki-repo" diff --cached --quiet; then
  echo "La wiki ya esta actualizada; no hay cambios para publicar."
  exit 0
fi

git -C "$tmp_dir/wiki-repo" commit -m "Update project wiki"
git -C "$tmp_dir/wiki-repo" push

echo "Wiki publicada correctamente."

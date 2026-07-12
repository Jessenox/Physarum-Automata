#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WIKI_SOURCE_DIR="$ROOT_DIR/wiki"

if [[ ! -d "$WIKI_SOURCE_DIR" ]]; then
  echo "No existe la carpeta wiki/: $WIKI_SOURCE_DIR" >&2
  exit 1
fi

origin_url="$(git -C "$ROOT_DIR" remote get-url origin)"

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

tmp_dir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmp_dir"
}
trap cleanup EXIT

echo "Publicando wiki en: $wiki_url"

if ! git clone "$wiki_url" "$tmp_dir/wiki-repo"; then
  echo "No se pudo clonar la wiki. Verifica que Wikis este habilitado en GitHub." >&2
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

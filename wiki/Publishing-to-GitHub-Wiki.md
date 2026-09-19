# Publishing to GitHub Wiki

Language: [[Espanol|Publicar-en-GitHub-Wiki]] | **English**

## Important

The GitHub **Wiki** tab does not automatically read the `wiki/` folder from the main repository.

GitHub Wiki uses a separate Git repository:

```text
git@github.com:OWNER/REPOSITORY.wiki.git
```

For this project:

```text
git@github.com:Jessenox/Physarum-Automata.wiki.git
```

Therefore, `git push origin main` only uploads the source files, not the Wiki tab contents.

## Recommended flow

1. Edit Markdown files inside `wiki/`.
2. Commit and push those changes to the main repository.
3. Run:

```bash
./scripts/publish-wiki.sh
```

4. Open the **Wiki** tab on GitHub.

## Requirements

- Wiki enabled in the GitHub repository.
- Write permission over `Jessenox/Physarum-Automata`.
- SSH key configured for GitHub push access.

## What the script does

The script:

- reads `origin`;
- calculates the `.wiki.git` remote;
- clones the wiki repository into a temporary folder;
- copies the contents of `wiki/`;
- creates a commit if there are changes;
- pushes to the wiki repository.

## If GitHub says the wiki repository does not exist

In GitHub:

1. open the repository;
2. open **Settings**;
3. find **Features**;
4. enable **Wikis**;
5. open the **Wiki** tab;
6. create any first page, for example `Home`;
7. run `./scripts/publish-wiki.sh` again.

Typical error:

```text
ERROR: Repository not found.
fatal: Could not read from remote repository.
```

If the main repository responds but `Physarum-Automata.wiki.git` does not, the remote is correct. The Wiki is missing, not initialized, or inaccessible.

## Explicit remotes

Default:

```bash
./scripts/publish-wiki.sh
```

Explicit main remote:

```bash
./scripts/publish-wiki.sh --remote git@github.com:Jessenox/Physarum-Automata.git
```

Explicit wiki remote:

```bash
./scripts/publish-wiki.sh --wiki-url git@github.com:Jessenox/Physarum-Automata.wiki.git
```


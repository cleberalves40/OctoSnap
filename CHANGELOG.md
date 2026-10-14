# Changelog

Todas as mudanças relevantes do OctoSnap são registradas aqui.
O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/)
e o projeto usa [versionamento semântico](https://semver.org/lang/pt-BR/).

## [0.1.1] - 2026-10-14

### Corrigido
- Job NetBSD do CI: instala os sets X11 (`xbase`, `xcomp`) na VM, necessários aos pacotes Qt6.
- `netbsd/Makefile`: `GITHUB_TAG` segue o formato de tag `X.Y.Z`.
- Substituída a sobrecarga deprecada de `QMenu::addAction` no menu Arquivo.

### Alterado
- A versão passa a ser definida apenas no `CMakeLists.txt` (`OCTOSNAP_VERSION`);
  a janela "Sobre" e `QApplication::applicationVersion()` usam esse valor.
- CI: execução manual (`workflow_dispatch`), `actions/checkout@v5` e verificação
  de consistência da versão entre CMake, pkgsrc e CHANGELOG.

## [0.1.0] - 2026-10-14

### Adicionado
- Primeira versão: listagem, criação, destruição e montagem somente leitura de
  snapshots FFS (`fss(4)`), com helper privilegiado via `doas`/`sudo`.
- Temas, tradução (en, pt_BR), CI no GitHub Actions, modelo de pacote pkgsrc e
  arquivo `.desktop`.

[0.1.1]: https://github.com/cleberalves40/OctoSnap/compare/0.1.0...0.1.1
[0.1.0]: https://github.com/cleberalves40/OctoSnap/releases/tag/0.1.0

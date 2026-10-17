# Changelog

Todas as mudanças relevantes do OctoSnap são registradas aqui.
O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/)
e o projeto usa [versionamento semântico](https://semver.org/lang/pt-BR/).

## [0.2.1] - 2026-10-17

### Corrigido
- `octosnap-helper` executava `/sbin/fssconfig`, mas o NetBSD instala o programa em
  `/usr/sbin/fssconfig`. Por isso `create`, `list` e `destroy` falhavam num sistema real.
- `octosnap-helper` lia errado a saída de `fssconfig -l` (`fss0: /ponto`) e nunca via uma
  unidade como ativa. Snapshots apareciam como obsoletos, `mount-ro` os recusava, um segundo
  snapshot tentava reutilizar `fss0` e `destroy` não liberava a unidade.

### Adicionado
- Homologação no NetBSD 10.2 em **amd64**, **aarch64**, **i386** e **earmv7hf** (ARMv7),
  com um job de CI por arquitetura. As quatro compilam sem warnings e sem nenhum ajuste
  específico de código ou de flags.
- `tests/netbsd-e2e.sh`: teste ponta a ponta do helper contra um dispositivo `fss(4)` real,
  mais um teste de inicialização da GUI. Roda em todas as arquiteturas.
- `docs/ARCHITECTURES.md`: resultados por arquitetura, problemas encontrados e correções, e
  ajustes avaliados e descartados.

### Alterado
- CI: jobs i386 e earmv7hf com o anita sob QEMU, cache de pacotes pkgsrc no host
  (`ci/pkg-proxy.py`) e verificação de versão também no `octosnap.pro`.

## [0.2.0] - 2026-10-16

### Adicionado
- Restauração de arquivos e diretórios a partir de um snapshot (ação **Restaurar**):
  diálogo com a árvore do snapshot, seleção múltipla e prévia comparando a versão
  do snapshot com a atual. Se o snapshot não estiver montado, oferece a montagem
  somente leitura antes de abrir o diálogo.
- Por padrão a versão atual de cada item é preservada como
  `<item>.octosnap-AAAAMMDD-HHMMSS`; sobrescrever exige confirmação adicional.
- Comando `restore <nome> [--rename-existing] <caminho>...` no `octosnap-helper`:
  percorre os caminhos com `openat(O_NOFOLLOW)`, não atravessa outros sistemas de
  arquivos, grava via arquivo temporário + `renameat` e preserva dono, modo e datas.
  Diretórios são mesclados; diretórios pais ausentes são recriados.

- Tradução completa de todas as mensagens (103 strings) para Português do Brasil (pt_BR) e Inglês (en_US).
- Suporte a compilação alternativa com QMake (`octosnap.pro`).
- Atalhos de teclado para montagem (<kbd>Ctrl+M</kbd>), desmontagem (<kbd>Ctrl+U</kbd>), exclusão (<kbd>Delete</kbd>) e filtro (<kbd>Ctrl+F</kbd>).

### Limitações conhecidas
- Hard links são restaurados como cópias independentes e arquivos esparsos são
  expandidos; dispositivos, FIFOs e sockets são ignorados.

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

[0.2.1]: https://github.com/cleberalves40/OctoSnap/compare/0.2.0...0.2.1
[0.2.0]: https://github.com/cleberalves40/OctoSnap/compare/0.1.1...0.2.0
[0.1.1]: https://github.com/cleberalves40/OctoSnap/compare/0.1.0...0.1.1
[0.1.0]: https://github.com/cleberalves40/OctoSnap/releases/tag/0.1.0

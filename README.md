# OctoSnap (C++/Qt6) – Gerenciador de Snapshots FFS (NetBSD)

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Framework](https://img.shields.io/badge/Framework-Qt6-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/Platform-NetBSD-orange.svg)](https://www.netbsd.org/)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-yellow.svg)](LICENSE)
[![Build & Test](https://github.com/cleberalves40/OctoSnap/actions/workflows/build.yml/badge.svg)](https://github.com/cleberalves40/OctoSnap/actions/workflows/build.yml)

Interface gráfica nativa em **C++ / Qt6** para os snapshots de sistema de arquivos do NetBSD ([`fss(4)`](https://man.netbsd.org/fss.4) / [`fssconfig(8)`](https://man.netbsd.org/fssconfig.8)), permitindo criar, inspecionar e remover snapshots FFS de forma rápida e segura.

> ⚠️ **Work in progress** – versão 0.1.1. A restauração de arquivos a partir de um snapshot está planejada para a próxima fase.

---

## 🏛️ Arquitetura

```text
OctoSnap/
├── CMakeLists.txt           # Build system (CMake ≥ 3.21, Qt ≥ 6.5)
├── octosnap.desktop         # Entrada para o menu de aplicativos desktop
├── netbsd/                  # Modelo de pacote para o pkgsrc
│   ├── Makefile
│   ├── DESCR
│   └── PLIST
├── .github/workflows/       # CI/CD automatizado via GitHub Actions
│   └── build.yml
├── config/                  # Exemplos de configuração de privilégios
│   ├── doas.conf.example
│   └── sudoers.d-octosnap
├── helper/
│   └── octosnap-helper.cpp  # Helper privilegiado (sem Qt, sem shell)
├── translations/            # Traduções Qt Linguist (en, pt_BR)
├── CHANGELOG.md             # Histórico de versões
├── README.md
├── LICENSE
└── src/
    ├── main.cpp             # Ponto de entrada (QApplication & i18n)
    ├── FssBackend.h/.cpp    # Integração com o helper via doas/sudo
    ├── MainWindow.h/.cpp    # Janela Qt6, tabela de snapshots e diálogos
    └── Theme.h/.cpp         # Temas (Nord, Catppuccin, Solarized, ...)
```

| Componente | Localização | Responsabilidade |
|:---|:---|:---|
| **Backend** | `src/FssBackend.*` | Lista sistemas FFS montados e chama o helper via `doas`/`sudo` |
| **Interface GUI** | `src/MainWindow.*` | Janela Qt6, filtro, abas Informação / Saída / Ajuda |
| **Temas** | `src/Theme.*` | Paletas e estilo Fusion |
| **Helper** | `helper/octosnap-helper.cpp` | Operações privilegiadas com lista fixa de comandos |
| **i18n** | `translations/*.ts` | Tradução (en, pt_BR) embutida como recurso Qt |

### Separação de privilégios

A interface **nunca roda como root**. Somente o `octosnap-helper` é executado com privilégios, através de `doas` (padrão) ou `sudo -A`. O helper valida todos os argumentos e executa apenas `/sbin/fssconfig`, `/sbin/mount` e `/sbin/umount` via `execv()`, sem shell:

```text
octosnap-helper list
octosnap-helper create  <nome> <ponto-de-montagem>
octosnap-helper destroy <nome> [--remove-backing]
octosnap-helper mount-ro <nome> <diretório>
octosnap-helper umount  <diretório>
```

O estado fica em `/var/db/octosnap` (metadados e arquivos de *backing store*).

---

## ✨ Funcionalidades

- **Listagem**: snapshots com unidade `fssN`, ponto de montagem, espaço ocupado, data de criação e estado (ativo, montado RO ou obsoleto).
- **Criação**: escolha do sistema de arquivos FFS e do nome do snapshot.
- **Montagem somente leitura**: inspecione o conteúdo de um snapshot em um diretório vazio.
- **Destruição**: com opção de remover também o arquivo de *backing store*.
- **Temas e idiomas**: alternância de tema em tempo real; idioma do sistema, inglês ou português.

---

## 🔨 Compilação e instalação

### Dependências (NetBSD)

```sh
pkgin install cmake ninja-build qt6-qtbase qt6-qttools
```

### Com CMake

```sh
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/pkg/qt6
cmake --build build
doas cmake --install build --prefix /usr/pkg
```

### Com pkgsrc

Copie o diretório `netbsd/` para `/usr/pkgsrc/sysutils/octosnap`, gere o `distinfo` e instale:

```sh
cd /usr/pkgsrc/sysutils/octosnap
make makesum
make install clean
```

### Configurar a elevação de privilégios

Use um dos exemplos instalados em `/usr/pkg/share/examples/octosnap/`:

```sh
# doas (padrão)
cat /usr/pkg/share/examples/octosnap/doas.conf.example >> /etc/doas.conf

# ou sudo
visudo -f /etc/sudoers.d/octosnap
```

---

## ⌨️ Atalhos de Teclado

| Atalho | Ação |
|:---|:---|
| <kbd>F5</kbd> | Atualizar lista de snapshots |
| <kbd>Ctrl+N</kbd> | Criar novo snapshot |
| <kbd>Ctrl+Q</kbd> | Sair |

---

## 🔄 Fluxo de trabalho

- O branch `main` é sempre compilável; mudanças entram via *pull request*.
- Todo push e PR para `main` executa o workflow **Build & Test**:
  - **Ubuntu / Qt 6.7**: compila a GUI com Clang (verificação rápida de portabilidade).
  - **NetBSD (VM)**: compila a GUI e o `octosnap-helper` com os pacotes Qt6 do pkgsrc.
- Releases são marcadas com tags `X.Y.Z` (ex.: `0.1.1`), que o `netbsd/Makefile` usa para baixar o código (`GITHUB_TAG`).
- Para uma nova versão: altere `project(... VERSION ...)` no `CMakeLists.txt` e `DISTNAME` no `netbsd/Makefile` (o CI verifica se coincidem), registre as mudanças no [CHANGELOG.md](CHANGELOG.md), faça o merge e crie a tag.

---

## 📄 Licença

Distribuído sob a licença [BSD 3-Clause](LICENSE).

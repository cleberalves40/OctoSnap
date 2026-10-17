# Arquiteturas suportadas

O OctoSnap é homologado no NetBSD 10.2 em quatro arquiteturas. Cada uma tem um job
próprio no workflow [Build & Test](../.github/workflows/build.yml), que compila a GUI e o
`octosnap-helper` com o toolchain do sistema e os pacotes Qt6 do pkgsrc e depois executa
[`tests/netbsd-e2e.sh`](../tests/netbsd-e2e.sh).

O teste ponta a ponta roda como root dentro da VM. Ele cria um FFS de rascunho num `vnd(4)`
e exercita o helper contra um dispositivo `fss(4)` real: criação, dois snapshots simultâneos
em unidades diferentes, listagem, montagem somente leitura, restauração com e sem backup,
recusa de caminhos inseguros e de diretório pai que seja symlink, desmontagem e destruição
com liberação da unidade. Por fim, verifica que a GUI inicia (`QT_QPA_PLATFORM=offscreen`).

## Resultados (0.2.1)

| Arquitetura | Job | Modelo | Binário gerado | Qt6 (pkgsrc) | Warnings | Testes |
|:---|:---|:---|:---|:---|:---:|:---:|
| amd64 | `netbsd-amd64` | LP64 | ELF 64-bit x86-64 | 6.11.2 | 0 | ✅ |
| aarch64 | `netbsd-aarch64` | LP64 | ELF 64-bit ARM aarch64 | 6.11.1 | 0 | ✅ |
| i386 | `netbsd-i386` | ILP32 | ELF 32-bit Intel 80386 | 6.11.2 | 0 | ✅ |
| earmv7hf | `netbsd-earmv7hf` | ILP32 | ELF 32-bit ARM EABI5 | 6.11.1 | 0 | ✅ |

Compilador: GCC 10.5.0 do sistema base em todas. Nenhuma arquitetura exigiu mudança de
código-fonte, flag de compilação ou de linker específica.

## Como cada job roda

| Arquitetura | Ferramenta | Execução |
|:---|:---|:---|
| amd64 | [vmactions/netbsd-vm](https://github.com/vmactions/netbsd-vm) | QEMU com KVM |
| aarch64 | vmactions/netbsd-vm (`arch: aarch64`) | QEMU emulado (TCG) no runner x86_64 |
| i386 | [anita](https://github.com/gson1703/anita), instalação via sysinst | QEMU com KVM |
| earmv7hf | anita, imagem oficial `armv7.img` | QEMU emulado (TCG), máquina `virt` |

Não existe action pronta para NetBSD i386 nem earmv7hf. Por isso esses dois jobs usam o anita,
a ferramenta de testes automatizados do próprio projeto NetBSD, por meio de
[`ci/anita-run.sh`](../ci/anita-run.sh). Os quatro jobs executam o mesmo
[`ci/netbsd-ci.sh`](../ci/netbsd-ci.sh) dentro da VM.

## Problemas encontrados e correções

### No código (afetavam todas as arquiteturas)

O teste ponta a ponta foi o primeiro a executar o helper num NetBSD real. Ele revelou dois
defeitos presentes desde a 0.1.0:

- **Caminho do `fssconfig`.** O helper executava `/sbin/fssconfig`, mas o NetBSD instala o
  programa em `/usr/sbin/fssconfig` (lista de sets `base/mi`). Com isso, `create`, `list` e
  `destroy` sempre falhavam. Corrigido em `kFssconfig`.
- **Leitura da saída de `fssconfig -l`.** O comando imprime `fss0: /ponto` ou
  `fss1: not in use`. O helper tomava `fss0:` (com os dois-pontos) como nome da unidade, que
  nunca passava em `validUnit`. Assim, nenhum snapshot aparecia como ativo, `mount-ro` os
  recusava, `freeUnit()` sempre escolhia `fss0` e `destroy` não executava `fssconfig -u`.
  Corrigido em `activeUnits()`.

### Por arquitetura

- **i386: atômicos de 64 bits.** O teste `HAVE_STDATOMIC` do Qt falha com e sem `libatomic`,
  e só nesta arquitetura. Provavelmente o GCC do NetBSD/i386 gera código para i486, que não
  tem `cmpxchg8b`, e o sistema base não traz `libatomic`. O Qt segue em frente
  (`Found WrapAtomic: TRUE`) e o OctoSnap não usa `std::atomic` de 64 bits, então nada é
  afetado hoje. Se código futuro precisar disso, compile com `-march=i586` ou superior, ou
  ligue com a `libatomic` do GCC do pkgsrc (`lang/gcc*`).
- **i386 e earmv7hf: formato dos sets.** Os sets de distribuição são `.tgz` nessas
  arquiteturas e `.tar.xz` em amd64 e aarch64. O CI tenta os dois formatos ao instalar
  `xbase` e `xcomp`.
- **earmv7hf: DTB do anita.** O anita exige que o DTB da máquina `vexpress-a15` exista mesmo
  com `--machine virt`, que gera a própria árvore de dispositivos. O job passa
  `--dtb /dev/null`.
- **aarch64 e earmv7hf: velocidade.** Ambos rodam em emulação TCG. Mesmo assim, com o cache
  de pacotes descrito abaixo, cada job leva de 10 a 15 minutos.

### Na infraestrutura de CI

- **Repositório de pacotes.** Os pacotes binários ficam em `NetBSD/<arch>/10.0/All` para toda
  a série 10.x. Esse diretório é um alias para o trimestral corrente (`10.0_2026Q3`). Os
  espelhos trimestrais que as imagens do vmactions configuram não tinham Qt6 para aarch64 e
  oscilaram no amd64. O CI usa sempre o repositório principal com `pkg_add -uu`, que também
  atualiza dependências antigas já instaladas na imagem (por exemplo, `pcre2`).
- **Downloads dentro das VMs do anita.** Pela rede NAT do QEMU, o `pkg_add` levou mais de
  duas horas no i386 e chegou a morrer com SIGPIPE ao reconectar, embora o sistema tivesse
  iniciado em três minutos. Agora o host baixa cada arquivo uma vez e o serve à VM por HTTP
  em `10.0.2.2` ([`ci/pkg-proxy.py`](../ci/pkg-proxy.py)). O tempo caiu para cerca de 8
  minutos no i386 e 12 no earmv7hf. A instalação também ignora SIGPIPE e tenta até três vezes.

## Ajustes avaliados e descartados

Foram propostos para 32 bits e não são necessários, segundo os resultados acima:

- `-Wl,--no-keep-memory` no linker e `UNLIMIT_RESOURCES+= datasize` no pacote: i386 e
  earmv7hf compilaram e ligaram sem esgotar memória, com os limites padrão do sistema.
- `_FILE_OFFSET_BITS=64`: sem efeito no NetBSD, onde `off_t` sempre tem 64 bits e `time_t`
  tem 64 bits desde o NetBSD 6.
- Ligação com `libatomic` em ARM 32 bits: no earmv7hf o teste de atômicos do Qt passa sem
  ela, e o sistema base não a fornece. O caso sensível é o i386 (veja acima).
- `-funsigned-char` em aarch64: `char` já é sem sinal por padrão na ABI ARM, e o código não
  depende do sinal de `char` (`validName` e `validUnit` convertem para `unsigned char`).

## Executando localmente

Num NetBSD de qualquer uma das quatro arquiteturas, como root e a partir da raiz do
código-fonte:

```sh
sh ci/netbsd-ci.sh                 # instala dependências, compila e testa
sh tests/netbsd-e2e.sh build       # só o teste, sobre um build existente
```

O teste usa `vnd0` e grava o backing store em `/var/db/octosnap`. Não o rode numa máquina
que já use `vnd0`.
